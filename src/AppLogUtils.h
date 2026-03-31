/*!
 * ********************************************************************************************************************************
 * @file        AppLogUtils.h
 * @brief       Logging and small text utilities for ProjectSource.
 * @details     Provides:
 *              - QTextEdit append helpers for log UI
 *              - Simple startup log cleanup
 *              - bash single-quote escaping helper used by WSL command composition
 *
 * @authors     Jeffrey Scott Flesher with the help of AI: Copilot
 * @date        2026-03-26
 *********************************************************************************************************************************/
#pragma once

#include <QString>

class QTextEdit;

namespace psd::log
{
    void clearLogFilesAtStartup();
    void logAppend(QTextEdit *output, const QString &text);

    /*!
     * ****************************************************************************************************************************
     * @brief   Escapes a string for inclusion inside a single-quoted bash string.
     * @param   s Input string.
     * @return  Escaped string safe for bash single-quoted contexts.
     *****************************************************************************************************************************/
    QString bashEscapeSingleQuoted(const QString &s);
} // namespace psd::log

/*!
 * ********************************************************************************************************************************
 * End of AppLogUtils.h
 *********************************************************************************************************************************/
