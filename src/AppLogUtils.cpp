/*!
 * ********************************************************************************************************************************
 * @file        AppLogUtils.cpp
 * @brief       Implements logging and small text utilities for ProjectSource.
 * @details     Implements psd::log helpers used by the UI and exporter.
 *
 * @authors     Jeffrey Scott Flesher with the help of AI: Copilot
 * @date        2026-03-31
 *********************************************************************************************************************************/

#include "AppLogUtils.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextCursor>
#include <QTextEdit>
#include <QTextStream>

namespace psd::log
{
    /*!
     * ****************************************************************************************************************************
     * @brief   Clears startup log files if they exist.
     * @details Writes empty contents to message-queue.txt and error-log.txt in the current working
     * directory. This is intentionally conservative: it does not guess a project root.
     *****************************************************************************************************************************/
    void clearLogFilesAtStartup()
    {
        const QString root = QDir::currentPath();

        {
            QFile file(QDir(root).filePath(QStringLiteral("message-queue.txt")));
            const bool opened = file.open(QIODevice::WriteOnly | QIODevice::Truncate);
            Q_UNUSED(opened);
        }

        {
            QFile file(QDir(root).filePath(QStringLiteral("error-log.txt")));
            const bool opened = file.open(QIODevice::WriteOnly | QIODevice::Truncate);
            Q_UNUSED(opened);
        }
    }

    /*!
     * ****************************************************************************************************************************
     * @brief   Appends text to a QTextEdit output console.
     *****************************************************************************************************************************/
    void logAppend(QPlainTextEdit *output, const QString &text)
    {
        if (!output) return;

        output->moveCursor(QTextCursor::End);
        output->insertPlainText(text);
        output->moveCursor(QTextCursor::End);
    }

    /*!
     * ****************************************************************************************************************************
     * @brief   Escapes a string for inclusion inside a single-quoted bash string.
     * @details Bash safe single-quote escaping:
     *          ' -> '\''   (close quote, escaped quote, reopen quote)
     *****************************************************************************************************************************/
    QString bashEscapeSingleQuoted(const QString &s)
    {
        QString out = s;
        out.replace('\'', QStringLiteral("'\\''"));
        return out;
    }
} // namespace psd::log

/*!
 * ********************************************************************************************************************************
 * End of AppLogUtils.cpp
 *********************************************************************************************************************************/
