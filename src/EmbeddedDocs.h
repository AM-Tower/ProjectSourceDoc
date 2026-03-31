/*!
 * ********************************************************************************************************************************
 * @file EmbeddedDocs.h
 * @brief Embedded documentation helpers (README loading and Usage dialog).
 * @authors     Jeffrey Scott Flesher using AI: Copilot
 * @date        2026-03-26
 *
 * @details
 * This header declares helper functions used to load and display embedded documentation bundled
 * with the application as Qt resources.
 *
 * Responsibilities:
 *  - Normalize the system locale to the documentation naming scheme.
 *  - Load locale-specific or fallback README markdown from resources.
 *  - Display the Usage dialog containing rendered markdown.
 *
 * These utilities were previously implemented as file-local static functions and have been
 * refactored into a namespace to allow reuse across translation units and tests.
 *
 * @namespace psd::docs
 *********************************************************************************************************************************/

#pragma once

#include <QString>

class QWidget;

namespace psd::docs
{
    /*!
     * ****************************************************************************************************************************
     * @brief Normalizes the system locale for documentation selection.
     *
     * Rules:
     *  - Preserve "zh_CN" explicitly.
     *  - Otherwise reduce to language only (e.g. "en_US" -> "en").
     *
     * @return Normalized locale identifier.
     *****************************************************************************************************************************/
    QString normalizedLocaleForDocs();

    /*!
     * ****************************************************************************************************************************
     * @brief Reads a UTF-8 text file from Qt resources.
     *
     * @param[in] path Resource path to read.
     * @param[out] outText Output string receiving file contents.
     * @return True on success; false if the resource could not be opened.
     *****************************************************************************************************************************/
    bool readResourceText(const QString &path, QString &outText);

    /*!
     * ****************************************************************************************************************************
     * @brief Loads the most appropriate embedded README markdown.
     *
     * Resolution order:
     *  1) Locale-specific README (:/README_<locale>.md)
     *  2) Base README (:/README.md)
     *
     * @return Markdown content to display in the Usage dialog.
     *****************************************************************************************************************************/
    QString loadReadmeMarkdown();

    /*!
     * ****************************************************************************************************************************
     * @brief Displays the Usage dialog containing embedded documentation.
     *
     * Renders embedded README markdown inside a modal dialog using QTextBrowser.
     *
     * @param[in] parent Parent widget for dialog ownership.
     *****************************************************************************************************************************/
    void showUsageDialog(QWidget *parent);

} // namespace psd::docs

/*!
 * ********************************************************************************************************************************
 * End of EmbeddedDocs.h
 *********************************************************************************************************************************/
