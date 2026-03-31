/*!
 * ********************************************************************************************************************************
 * @file         ProjectSourceExporter.h
 * @brief        Declares ProjectSourceExporter helper for generating Project‑Source.txt and
 * backups.
 * @details      ProjectSourceExporter is a stateless utility class that performs:
 *
 *               - Project source export via cmake/PackSource.cmake
 *               - Timestamped project backups with exclusion rules
 *
 *               The exporter never discovers project roots on its own; all paths are
 *               supplied explicitly by the caller (typically MainWindow).
 *
 * @author       Jeffrey Scott Flesher
 * @date         2026-03-31
 *********************************************************************************************************************************/
#pragma once

#include <QString>
#include <QStringList>

/* Forward declarations */
class QWidget;
class QStatusBar;

/*!
 * ********************************************************************************************************************************
 * @class        ProjectSourceExporter
 * @brief        Static helper for project source generation and backups.
 *
 * @details      This class exposes only static functions and maintains no internal state.
 *               It is responsible for:
 *
 *               - Validating project structure
 *               - Invoking cmake/PackSource.cmake
 *               - Reporting progress and failures
 *               - Creating timestamped backups of entire projects
 *
 * @public
 *********************************************************************************************************************************/
class ProjectSourceExporter final
{
    public:
        ProjectSourceExporter() = delete;
        /*!
         * ************************************************************************************************************************
         * @brief        Generates Project‑Source.txt using default include/exclude rules.
         * @details      Backward‑compatible overload for legacy call sites.
         *
         *               Defaults:
         *               - Include: cpp; h; md
         *               - Exclude: build*; .rcc; CMakeFiles; .git; .qtcreator; engines; 0‑Archive
         *
         * @param[in]    parent         Parent widget for dialogs (may be null).
         * @param[in]    status         Optional status bar for progress messages.
         * @param[in]    projectRoot    Absolute path to the project root.
         * @param[out]   outProcessLog  Optional destination for captured process output.
         *
         * @return       Absolute path to Project‑Source.txt on success; empty string on failure.
         *************************************************************************************************************************/
        static QString generateAndReturnPath(QWidget *parent, QStatusBar *status, const QString &projectRoot, QString *outProcessLog = nullptr);

        /*!
         * ************************************************************************************************************************
         * @brief        Generates Project‑Source.txt using explicit include/exclude rules.
         *
         * @param[in]    parent         Parent widget for dialogs (may be null).
         * @param[in]    status         Optional status bar for progress messages.
         * @param[in]    projectRoot    Absolute path to the project root.
         * @param[in]    includeExts    File extensions to include.
         * @param[in]    excludeDirs    Folder exclusion patterns.
         * @param[out]   outProcessLog  Optional destination for captured process output.
         *
         * @return       Absolute path to Project‑Source.txt on success; empty string on failure.
         *************************************************************************************************************************/
        static QString generateAndReturnPath(QWidget *parent, QStatusBar *status, const QString &projectRoot, const QStringList &includeExts, const QStringList &excludeDirs, QString *outProcessLog = nullptr);

        /*!
         * ************************************************************************************************************************
         * @brief        Generates Project‑Source.txt and shows a success dialog.
         *
         * @param[in]    parent         Parent widget for dialogs.
         * @param[in]    status         Optional status bar for progress messages.
         * @param[in]    projectRoot    Absolute path to the project root.
         *
         * @return       True on success; false on failure.
         *************************************************************************************************************************/
        static bool generate(QWidget *parent, QStatusBar *status, const QString &projectRoot);

        /*!
         * ************************************************************************************************************************
         * @brief        Creates a timestamped backup of the entire project.
         *
         * @details      Copies all files and folders under @p projectRoot into a new,
         *               timestamped destination folder.
         *
         *               - The backup is created under @p backupBaseFolder
         *               - If empty, <projectRoot>/0‑Backup is used
         *               - Folder exclusion rules are applied
         *               - Symlinks are skipped to avoid recursion
         *
         * @param[in]    parent            Parent widget for dialogs.
         * @param[in]    status            Optional status bar for progress messages.
         * @param[in]    projectRoot       Absolute path to the project root.
         * @param[in]    excludeDirs       Folder exclusion patterns.
         * @param[in]    backupBaseFolder  Base folder for backups.
         * @param[out]   outBackupLog      Optional destination for backup log output.
         *
         * @return       Absolute path to the timestamped backup folder; empty on failure.
         *************************************************************************************************************************/
        static QString backupProjectToTimestampedFolder(QWidget *parent, QStatusBar *status, const QString &projectRoot, const QStringList &excludeDirs, const QString &backupBaseFolder, QString *outBackupLog);

        /*!
         * ****************************************************************************************************************************
         * @brief        Generates a tree-style listing of the project directory.
         *
         * @details      Respects include extensions and excluded directories. Output is written
         *               to Project-Tree.txt in the project root. This function is stateless and
         *               uses only the parameters provided by the caller.
         *
         * @param parent        Parent widget for message boxes (optional).
         * @param status        Status bar for progress messages (optional).
         * @param projectRoot   Absolute path to the project root.
         * @param includeExts   File extensions to include (e.g., {"cpp","h","md"}).
         * @param excludeDirs   Directory patterns to exclude (e.g., {"build*",".git","engines"}).
         * @param outLog        Optional log output.
         *
         * @return Absolute path to Project-Tree.txt, or empty on failure.
         *****************************************************************************************************************************/
        static QString createTree(QWidget *parent, QStatusBar *status, const QString &projectRoot, const QStringList &includeExts, const QStringList &excludeDirs, QString *outLog = nullptr);

};

/*!
 * ********************************************************************************************************************************
 * @file         ProjectSourceExporter.h
 * @brief        End of ProjectSourceExporter.h.
 *********************************************************************************************************************************/
