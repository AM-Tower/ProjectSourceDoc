/*!
 * ********************************************************************************************************************************
 * @file StatusBarQueue.h
 * @brief Thread-safe queued status bar message dispatcher.
 * @authors     Jeffrey Scott Flesher using AI: Copilot
 * @date        2026-03-31
 *
 * This file defines StatusBarQueue, a small utility class that queues status messages with
 * per-message durations. Messages can be enqueued from any thread and are displayed sequentially
 * without blocking the UI thread.
 *
 * In addition to displaying messages, the dispatcher persists every enqueued message to:
 *   <project-root>/message-queue.txt
 *
 * The project root is resolved by walking upward until CMakeLists.txt is found.
 *********************************************************************************************************************************/
#pragma once

#include <QMutex>
#include <QObject>
#include <QQueue>
#include <QString>

class QStatusBar;
class QTimer;

/*!
 * ********************************************************************************************************************************
 * @class StatusBarQueue
 * @brief Thread-safe queued status bar message dispatcher.
 *********************************************************************************************************************************/
class StatusBarQueue final : public QObject
{
        Q_OBJECT

    public:
        /*!
         * ************************************************************************************************************************
         * @brief Constructs the queue dispatcher.
         *
         * @param statusBar Target status bar to display messages.
         * @param parent QObject parent for lifetime management.
         *************************************************************************************************************************/
        explicit StatusBarQueue(QStatusBar *statusBar, QObject *parent = nullptr);

        /*!
         * ************************************************************************************************************************
         * @brief Destroys the queue dispatcher.
         *************************************************************************************************************************/
        ~StatusBarQueue() override;

        /*!
         * ************************************************************************************************************************
         * @brief Enqueues a message to be shown for a given number of seconds.
         *
         * This function is thread-safe and may be called from worker threads.
         *
         * Every message is also appended to <project-root>/message-queue.txt with an ISO timestamp.
         *
         * @param message Status text to show.
         * @param seconds Number of seconds to display the message (default: 3).
         *************************************************************************************************************************/
        void enqueue(const QString &message, int seconds = 3);

    private slots:
        /*!
         * ************************************************************************************************************************
         * @brief Displays the next queued message, if any.
         *************************************************************************************************************************/
        void showNext();

    private:
        /*!
         * ************************************************************************************************************************
         * @brief Appends an enqueued message to the persistent message log.
         *
         * The log file is <project-root>/message-queue.txt.
         *
         * @param message Message text to append.
         *************************************************************************************************************************/
        void appendToMessageLog(const QString &message);

    private:
        struct Item
        {
                QString text;
                int msec;
        };

        QStatusBar *m_statusBar{nullptr};
        QQueue<Item> m_queue;
        QMutex m_mutex;
        QTimer *m_timer{nullptr};
};

/**********************************************************************************************************************************
 * End of StatusBarQueue.h
 *********************************************************************************************************************************/
