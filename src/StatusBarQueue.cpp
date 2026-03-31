/*!
 * ********************************************************************************************************************************
 * @file StatusBarQueue.cpp
 * @brief Implements the queued status bar message dispatcher.
 * @authors     Jeffrey Scott Flesher using AI: Copilot
 * @date        2026-03-26
 * @details
 * This file implements StatusBarQueue, which provides a thread-safe message queue for QStatusBar.
 * Messages are displayed sequentially for a caller-defined duration (seconds) without blocking the
 * UI.
 *
 * Every enqueued message is also appended to <project-root>/message-queue.txt with an ISO
 * timestamp. The project root is located by walking upward until CMakeLists.txt is found.
 *********************************************************************************************************************************/
#include "StatusBarQueue.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMetaObject>
#include <QMutex>
#include <QMutexLocker>
#include <QStatusBar>
#include <QTextStream>
#include <QTimer>

static QMutex g_messageLogMutex;

/*!
 * ********************************************************************************************************************************
 * @brief Locates the project root by walking upward until CMakeLists.txt is found.
 *
 * @param startDir Directory to start searching from.
 * \return Absolute path to project root, or empty string if not found.
 *********************************************************************************************************************************/
static QString findProjectRootFrom(const QString &startDir)
{
    QDir dir(startDir);
    while (dir.exists())
    {
        if (QFileInfo::exists(dir.filePath(QStringLiteral("CMakeLists.txt"))))
        {
            return dir.absolutePath();
        }
        if (!dir.cdUp())
        {
            break;
        }
    }
    return QString();
}

/*!
 * ********************************************************************************************************************************
 * @brief Resolves the message log file path (<project-root>/message-queue.txt).
 *
 * \return Absolute file path to message-queue.txt. Falls back to current directory if root cannot
 * be found.
 *********************************************************************************************************************************/
static QString messageQueueLogPath()
{
    QString root = findProjectRootFrom(QDir::currentPath());
    if (root.isEmpty())
    {
        root = findProjectRootFrom(QCoreApplication::applicationDirPath());
    }
    if (root.isEmpty())
    {
        root = QDir::currentPath();
    }
    return QDir(root).filePath(QStringLiteral("message-queue.txt"));
}

/*!
 * ********************************************************************************************************************************
 * @brief Constructs the queued status bar dispatcher.
 *
 * Initializes a single-shot timer that advances the queue when the current message expires.
 * The timer runs on the QObject thread that owns this instance (typically the UI thread).
 *
 * @param statusBar Target status bar used to display messages.
 * @param parent Parent QObject for lifetime management.
 *********************************************************************************************************************************/
StatusBarQueue::StatusBarQueue(QStatusBar *statusBar, QObject *parent) :
QObject(parent), m_statusBar(statusBar), m_timer(new QTimer(this))
{
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &StatusBarQueue::showNext);
}

/*!
 * ********************************************************************************************************************************
 * @brief Destroys the queued status bar dispatcher.
 *
 * QTimer is owned by this object and is cleaned up automatically by Qt parent/child ownership.
 *********************************************************************************************************************************/
StatusBarQueue::~StatusBarQueue() = default;

/*!
 * ********************************************************************************************************************************
 * @brief Enqueues a message to be shown for a given number of seconds.
 *
 * This function is thread-safe and may be called from worker threads.
 * The actual UI update is scheduled via a queued invocation so it executes on the UI thread.
 *
 * Every message is also appended to <project-root>/message-queue.txt with an ISO timestamp.
 *
 * @param message Status text to display.
 * @param seconds Duration to display the message (seconds). Values < 0 are treated as 0.
 *********************************************************************************************************************************/
void StatusBarQueue::enqueue(const QString &message, int seconds)
{
    if (seconds < 0) { seconds = 0; }

    appendToMessageLog(message);

    if (!m_statusBar) { return; }

    {
        QMutexLocker lock(&m_mutex);
        m_queue.enqueue({message, seconds * 1000});
    }

    QMetaObject::invokeMethod(this, &StatusBarQueue::showNext, Qt::QueuedConnection);
}

/*!
 * ********************************************************************************************************************************
 * @brief Appends an enqueued message to the persistent message log.
 *
 * The log file is <project-root>/message-queue.txt.
 *
 * @param message Message text to append.
 *********************************************************************************************************************************/
void StatusBarQueue::appendToMessageLog(const QString &message)
{
    QMutexLocker lock(&g_messageLogMutex);

    QFile f(messageQueueLogPath());
    if (!f.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
    {
        return;
    }

    QTextStream ts(&f);
    ts.setEncoding(QStringConverter::Utf8);
    ts << QDateTime::currentDateTime().toString(Qt::ISODate) << " | " << message << "\n";
}

/*!
 * ********************************************************************************************************************************
 * @brief Displays the next queued message if the status bar is idle.
 *
 * If a message is currently being displayed (timer active), this function returns immediately.
 * Otherwise, it dequeues one item and shows it for the configured duration.
 *
 * Notes:
 * - This function is intentionally conservative: it only advances when the timer is not active.
 * - When duration is 0, it immediately schedules the next dequeue operation.
 *********************************************************************************************************************************/
void StatusBarQueue::showNext()
{
    if (!m_statusBar)
    {
        return;
    }

    Item item;
    bool haveItem = false;

    {
        QMutexLocker lock(&m_mutex);

        if (m_timer->isActive())
        {
            return;
        }

        if (!m_queue.isEmpty())
        {
            item = m_queue.dequeue();
            haveItem = true;
        }
    }

    if (!haveItem)
    {
        return;
    }

    m_statusBar->showMessage(item.text, item.msec);

    if (item.msec > 0)
    {
        m_timer->start(item.msec);
    }
    else
    {
        QMetaObject::invokeMethod(this, &StatusBarQueue::showNext, Qt::QueuedConnection);
    }
}

/****************************************************************************************************************************
 * End of StatusBarQueue.cpp
 ****************************************************************************************************************************/
