/*!
 * ********************************************************************************************************************************
 * @file         ProjectSourceExporter.cpp
 * @brief        Implements ProjectSourceExporter utilities for project source generation and
 * backups.
 * @details      This file provides the full implementation of ProjectSourceExporter, including:
 *
 *               - Generation of Project‑Source.txt via cmake/PackSource.cmake
 *               - Timestamped backups of the entire project tree
 *               - Status bar progress reporting
 *               - Robust error handling and logging
 *
 *               The exporter is intentionally stateless. All state and configuration are supplied
 *               explicitly by the caller (typically MainWindow).
 *
 * @author       Jeffrey Scott Flesher
 * @date         2026-04-01
 *********************************************************************************************************************************/

#include "ProjectSourceExporter.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QStatusBar>
#include <QTimer>
#include <QUrl>
#include <utility> // for std::as_const

#include "ProjectPaths.h"

namespace
{
    static constexpr bool kDebugStatus = true;
    /*!
     * ****************************************************************************************************************************
     * @brief   debug Log Path For Project
     *****************************************************************************************************************************/
    static QString debugLogPathForProject(const QString &projectRoot)
    {
        return QDir(projectRoot).filePath(QStringLiteral("ProjectSource-Debug.log"));
    }

    /*!
     * ****************************************************************************************************************************
     * @brief   append Debug
     *****************************************************************************************************************************/
    static void appendDebug(const QString &projectRoot, const QString &msg)
    {
        QFile f(debugLogPathForProject(projectRoot));
        if (f.open(QIODevice::Append | QIODevice::Text))
        {
            QTextStream ts(&f);
            ts << QDateTime::currentDateTime().toString(Qt::ISODate) << "  " << msg << "\n";
        }
    }

    /*!
     * ****************************************************************************************************************************
     * @brief        Returns default include extensions for backward‑compatible calls.
     *****************************************************************************************************************************/
    QStringList defaultIncludeExtensions()
    {
        return {QStringLiteral("cpp"), QStringLiteral("h"), QStringLiteral("md")};
    }

    /*!
     * ****************************************************************************************************************************
     * @brief        Returns default excluded directory patterns for backward‑compatible calls.
     *****************************************************************************************************************************/
    QStringList defaultExcludeDirs()
    {
        return {QStringLiteral("build*"),     QStringLiteral(".rcc"), QStringLiteral("CMakeFiles"), QStringLiteral(".git"),
                QStringLiteral(".qtcreator"), QStringLiteral("engines"), QStringLiteral("0-Archive")};
    }

    /*!
     * ****************************************************************************************************************************
     * @brief        Posts a message to the status bar (if provided).
     *****************************************************************************************************************************/
    void postStatus(QStatusBar *status, const QString &message, int timeoutMs = 0)
    {
        if (kDebugStatus) { qDebug().noquote() << QDateTime::currentDateTime().toString(Qt::ISODate) << "[ProjectSourceExporter]" << message; }
        if (!status) return;
        if (timeoutMs > 0) status->showMessage(message, timeoutMs);
        else               status->showMessage(message);
        status->repaint();
        qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    }

    /*!
     * ****************************************************************************************************************************
     * @brief   append Debug Line
     *****************************************************************************************************************************/
    static void appendDebugLine(const QString &projectRoot, const QString &line)
    {
        QFile f(QDir(projectRoot).filePath(QStringLiteral("ProjectSource-Debug.log")));
        if (!f.open(QIODevice::Append | QIODevice::Text)) return;

        QTextStream ts(&f);
        ts << QDateTime::currentDateTime().toString(Qt::ISODate) << "  " << line << "\n";
    }

    /*!
     * ****************************************************************************************************************************
     * @brief        Checks whether a path or name matches any exclusion pattern.
     *****************************************************************************************************************************/
    bool matchAnyExclude(const QString &relPath, const QString &name, const QStringList &excludePatterns)
    {
        for (const QString &pat : excludePatterns)
        {
            if (QDir::match(pat, name))    return true;
            if (QDir::match(pat, relPath)) return true;
        }
        return false;
    }

    /*!
     * ****************************************************************************************************************************
     * @brief        Copies a file while preserving permissions.
     *****************************************************************************************************************************/
    bool copyFilePreserve(const QString &srcFile, const QString &dstFile, QString *log)
    {
        QFileInfo sfi(srcFile);
        QDir().mkpath(QFileInfo(dstFile).absolutePath());
        if (QFileInfo::exists(dstFile))
        {
            if (!QFile::remove(dstFile))
            {
                if (log) *log += QObject::tr("ERROR: Unable to remove existing file: %1\n").arg(dstFile);
                return false;
            }
        }
        if (!QFile::copy(srcFile, dstFile))
        {
            if (log) *log += QObject::tr("ERROR: Copy failed: %1 -> %2\n").arg(srcFile, dstFile);
            return false;
        }
        QFile::setPermissions(dstFile, sfi.permissions());
        return true;
    }

    /*!
     * ****************************************************************************************************************************
     * @brief        Recursively copies a directory tree with exclusion rules.
     *****************************************************************************************************************************/
    bool copyDirRecursive(const QDir &rootDir, const QString &srcAbsDir, const QString &dstAbsDir, const QStringList &excludeDirs, const QString &dstAbsRoot, QString *log, int *copiedFiles, int *skippedEntries, int *errors)
    {
        QDir src(srcAbsDir);
        if (!src.exists())
        {
            if (log)     *log += QObject::tr("ERROR: Source directory does not exist: %1\n").arg(srcAbsDir);
            if (errors) (*errors)++;
            return false;
        }
        QDir().mkpath(dstAbsDir);
        const QFileInfoList entries = src.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System, QDir::DirsFirst | QDir::Name);
        bool okAll = true;
        for (const QFileInfo &fi : entries)
        {
            const QString abs = fi.absoluteFilePath();
            if (!dstAbsRoot.isEmpty() && abs.startsWith(dstAbsRoot))
            {
                if (skippedEntries) (*skippedEntries)++;
                continue;
            }
            if (fi.isSymLink())
            {
                if (skippedEntries) (*skippedEntries)++;
                continue;
            }
            const QString rel = rootDir.relativeFilePath(abs);
            const QString name = fi.fileName();
            if (fi.isDir())
            {
                if (matchAnyExclude(rel, name, excludeDirs))
                {
                    if (skippedEntries) (*skippedEntries)++;
                    continue;
                }
                const QString dstChild = QDir(dstAbsDir).filePath(name);
                if (!copyDirRecursive(rootDir, abs, dstChild, excludeDirs, dstAbsRoot, log, copiedFiles, skippedEntries, errors)) { okAll = false; }
            }
            else
            {
                const QString dstFile = QDir(dstAbsDir).filePath(name);
                if (copyFilePreserve(abs, dstFile, log))
                {
                    if (copiedFiles) (*copiedFiles)++;
                }
                else
                {
                    okAll = false;
                    if (errors) (*errors)++;
                }
            }
        }
        return okAll;
    }


    /*!
     * ****************************************************************************************************************************
     * @brief Loads include/exclude/backup settings for a project, supporting multiple key schemes.
     * @details Tries keys based on:
     *          1) projectRoot literal
     *          2) MD5(projectRoot)
     * @param[in] projectRoot Absolute project root.
     * @param[out] outIncludeExts Include extension list (may remain empty if none stored).
     * @param[out] outExcludeDirs Exclude dir list (may remain empty if none stored).
     * @param[out] outBackupBase Backup base folder (may be empty).
     *****************************************************************************************************************************/
    void loadProjectSettings(const QString &projectRoot, QStringList &outIncludeExts, QStringList &outExcludeDirs, QString &outBackupBase)
    {
        QSettings s;

        const QString key1 = projectRoot;
        const QString key2 = psd::paths::projectIdForPath(projectRoot);

        auto readList = [&](const QString &key, const QString &field) -> QStringList
        {
            return s.value(QStringLiteral("project/%1/%2").arg(key, field)).toStringList();
        };

        auto readStr = [&](const QString &key, const QString &field) -> QString
        {
            return s.value(QStringLiteral("project/%1/%2").arg(key, field)).toString();
        };

        outIncludeExts = readList(key1, QStringLiteral("includeExts"));
        if (outIncludeExts.isEmpty()) outIncludeExts = readList(key2, QStringLiteral("includeExts"));

        outExcludeDirs = readList(key1, QStringLiteral("excludeDirs"));
        if (outExcludeDirs.isEmpty()) outExcludeDirs = readList(key2, QStringLiteral("excludeDirs"));

        outBackupBase = readStr(key1, QStringLiteral("backupFolder"));
        if (outBackupBase.trimmed().isEmpty()) outBackupBase = readStr(key2, QStringLiteral("backupFolder"));
    }


} // namespace

/*!
 * ********************************************************************************************************************************
 * @brief        Creates a timestamped backup of the entire project.
 *********************************************************************************************************************************/
QString ProjectSourceExporter::backupProjectToTimestampedFolder(QWidget *parent, QStatusBar *status, const QString &projectRoot, const QStringList &excludeDirs, const QString &backupBaseFolder, QString *outBackupLog)
{
    QString log;
    if (projectRoot.trimmed().isEmpty() || !QDir(projectRoot).exists())
    {
        log += QObject::tr("ERROR: Invalid project root: %1\n").arg(projectRoot);
        if (outBackupLog) *outBackupLog = log;
        return {};
    }
    QString base = backupBaseFolder.trimmed();
    if (base.isEmpty()) base = QDir(projectRoot).filePath(QStringLiteral("0-Backup"));
    const QString projectName = QFileInfo(projectRoot).fileName().trimmed().isEmpty() ? QStringLiteral("Project") : QFileInfo(projectRoot).fileName();
    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_HH-mm-ss"));
    const QString destRoot = QDir(base).filePath(projectName);
    const QString destFolder = QDir(destRoot).filePath(stamp);
    log += QObject::tr("[Backup] Project Root: %1\n").arg(projectRoot);
    log += QObject::tr("[Backup] Base Folder : %1\n").arg(base);
    log += QObject::tr("[Backup] Target      : %1\n").arg(destFolder);
    log += QObject::tr("[Backup] Excludes    : %1\n\n").arg(excludeDirs.join(';'));
    if (!QDir().mkpath(destFolder))
    {
        log += QObject::tr("ERROR: Unable to create backup destination folder: %1\n").arg(destFolder);
        if (outBackupLog) *outBackupLog = log;
        QMessageBox::warning(parent, QObject::tr("Backup"), QObject::tr("Unable to create backup folder:\n%1").arg(destFolder));
        postStatus(status, QObject::tr("Backup failed: cannot create destination."), 5000);
        return {};
    }
    postStatus(status, QObject::tr("Backing up project files..."));
    int copied = 0;
    int skipped = 0;
    int errors = 0;
    const QDir rootDir(projectRoot);
    const QString dstAbsRoot = QDir(destFolder).absolutePath();
    const bool ok = copyDirRecursive(rootDir, QDir(projectRoot).absolutePath(), dstAbsRoot, excludeDirs, dstAbsRoot, &log, &copied, &skipped, &errors);
    log += QObject::tr("\n[Backup] Done. copied=%1 skipped=%2 errors=%3 ok=%4\n").arg(copied).arg(skipped).arg(errors).arg(ok ? QStringLiteral("true") : QStringLiteral("false"));
    if (outBackupLog) *outBackupLog = log;
    postStatus(status, ok ? QObject::tr("Backup complete: %1").arg(destFolder) : QObject::tr("Backup completed with errors (see Log tab)."), 5000);
    return destFolder;
}

/*! ******************************************************************************************************************************
 * @brief       Generates Project‑Source.txt and returns the output path (defaults enforced + .gitignore).
 * @details     Single public entry point:
 *              - Applies default include extensions and default excluded directory patterns
 *              - Merges user settings (if present)
 *              - Merges selected project's .gitignore directory rules (always)
 *              - Invokes cmake/PackSource.cmake
 *              - Validates output file exists
 *              - Creates a timestamped backup using the same exclusion rules
 *
 * @param[in]   parent Parent widget for dialogs (may be null).
 * @param[in]   status Optional status bar for progress messages.
 * @param[in]   projectRoot Absolute path to the project root.
 * @param[out]  outProcessLog Optional destination for captured process output.
 *
 * @return      Absolute path to Project‑Source.txt on success; empty string on failure.
 ******************************************************************************************************************************* */
QString ProjectSourceExporter::generateAndReturnPath(QWidget* parent, QStatusBar* status, const QString& projectRoot, QString* outProcessLog)
{
    appendDebug(projectRoot, "=== generateAndReturnPath ENTER ===");
    appendDebug(projectRoot, QString("projectRoot=%1").arg(projectRoot));

    if (projectRoot.trimmed().isEmpty())
    {
        appendDebug(projectRoot, "ERROR: projectRoot is empty");
        return {};
    }

    if (!QDir(projectRoot).exists())
    {
        appendDebug(projectRoot, "ERROR: projectRoot does not exist");
        return {};
    }

    QString log;
    postStatus(status, QObject::tr("Generating Project‑Source.txt..."));

    // Defaults
    QStringList includeExts = defaultIncludeExtensions();
    QStringList settingsInclude;
    QStringList settingsExclude;
    QString backupBase;

    appendDebug(projectRoot, "Loading project settings");
    loadProjectSettings(projectRoot, settingsInclude, settingsExclude, backupBase);

    appendDebug(projectRoot, QString("settingsInclude=%1").arg(settingsInclude.join(',')));
    appendDebug(projectRoot, QString("settingsExclude=%1").arg(settingsExclude.join(',')));
    appendDebug(projectRoot, QString("backupBase=%1").arg(backupBase));

    if (!settingsInclude.isEmpty()) includeExts = settingsInclude;

    appendDebug(projectRoot, QString("effective includeExts=%1").arg(includeExts.join(',')));

    // Deterministic merge: defaults + user excludes + .gitignore
    QStringList effectiveExcludeDirs = defaultExcludeDirs();

    for (const QString& d : std::as_const(settingsExclude))
    {
        const QString v = d.trimmed();
        if (!v.isEmpty() && !effectiveExcludeDirs.contains(v)) effectiveExcludeDirs.append(v);
    }

    const QStringList gitignoreDirs = excludeDirsFromGitignore(projectRoot);
    for (const QString& d : std::as_const(gitignoreDirs))
    {
        const QString v = d.trimmed();
        if (!v.isEmpty() && !effectiveExcludeDirs.contains(v)) effectiveExcludeDirs.append(v);
    }

    appendDebug(projectRoot, QString("effectiveExcludeDirs=%1").arg(effectiveExcludeDirs.join(',')));

    // MUST be a real project root
    const QString cmakeListsPath = QDir(projectRoot).filePath(QStringLiteral("CMakeLists.txt"));

    appendDebug(projectRoot, QString("CMakeLists.txt=%1 exists=%2").arg(cmakeListsPath).arg(QFileInfo::exists(cmakeListsPath)));

    if (!QFileInfo::exists(cmakeListsPath))
    {
        log += QObject::tr("ERROR: Selected project has no CMakeLists.txt:\n%1\n").arg(projectRoot);
        if (outProcessLog) *outProcessLog = log;
        appendDebug(projectRoot, "ERROR: Missing CMakeLists.txt");
        return {};
    }

    // Absolute script path
    const QString scriptPath = QDir(projectRoot).filePath(QStringLiteral("cmake/PackSource.cmake"));

    appendDebug(projectRoot, QString("PackSource.cmake=%1 exists=%2").arg(scriptPath).arg(QFileInfo::exists(scriptPath)));

    if (!QFileInfo::exists(scriptPath))
    {
        log += QObject::tr("ERROR: PackSource.cmake not found:\n%1\n").arg(scriptPath);
        if (outProcessLog) *outProcessLog = log;
        appendDebug(projectRoot, "ERROR: Missing PackSource.cmake");
        return {};
    }

    // Invoke PackSource.cmake
    QProcess proc;
    proc.setProgram(QStringLiteral("cmake"));

    QStringList args;
    args << QStringLiteral("-DPROJECT_ROOT=%1").arg(projectRoot)
         << QStringLiteral("-DINCLUDE_EXTENSIONS=%1").arg(includeExts.join(';'))
         << QStringLiteral("-DEXCLUDE_DIRS=%1").arg(effectiveExcludeDirs.join(';'))
         << QStringLiteral("-P")
         << scriptPath;

    appendDebug(projectRoot, QString("cmake program=%1").arg(proc.program()));
    appendDebug(projectRoot, QString("cmake args=%1").arg(args.join(' ')));
    appendDebug(projectRoot, QString("cmake workingDir=%1").arg(projectRoot));

    proc.setArguments(args);
    proc.setWorkingDirectory(projectRoot);
    proc.start();

    if (!proc.waitForFinished(-1))
    {
        log += QObject::tr("ERROR: cmake process did not finish.\n");
        if (outProcessLog) *outProcessLog = log;
        appendDebug(projectRoot, "ERROR: cmake did not finish");
        return {};
    }

    const QString stdOut = QString::fromUtf8(proc.readAllStandardOutput());
    const QString stdErr = QString::fromUtf8(proc.readAllStandardError());

    appendDebug(projectRoot, QString("cmake exitCode=%1 exitStatus=%2").arg(proc.exitCode()).arg(proc.exitStatus() == QProcess::NormalExit ? "NormalExit" : "Crash"));

    if (!stdOut.trimmed().isEmpty()) appendDebug(projectRoot, QString("cmake stdout:\n%1").arg(stdOut));
    if (!stdErr.trimmed().isEmpty()) appendDebug(projectRoot, QString("cmake stderr:\n%1").arg(stdErr));

    log += stdOut;
    log += stdErr;

    if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0)
    {
        log += QObject::tr("ERROR: Project‑Source generation failed.\n");
        if (outProcessLog) *outProcessLog = log;
        appendDebug(projectRoot, "ERROR: cmake returned failure");
        return {};
    }

    const QString outPath = QDir(projectRoot).filePath(QStringLiteral("Project-Source.txt"));

    appendDebug(projectRoot, QString("Project-Source.txt=%1 exists=%2") .arg(outPath).arg(QFileInfo::exists(outPath)));

    if (!QFileInfo::exists(outPath))
    {
        log += QObject::tr("ERROR: Project‑Source.txt was not created.\n");
        if (outProcessLog) *outProcessLog = log;
        appendDebug(projectRoot, "ERROR: Output file missing");
        return {};
    }

    // Backup
    postStatus(status, QObject::tr("Project‑Source generated. Creating backup..."));
    appendDebug(projectRoot, "Backup: starting");

    QString backupLog;
    const QString backupFolder =
        ProjectSourceExporter::backupProjectToTimestampedFolder(
            parent, status, projectRoot,
            effectiveExcludeDirs, backupBase, &backupLog);

    appendDebug(projectRoot, QString("Backup folder=%1").arg(backupFolder));

    if (!backupLog.trimmed().isEmpty()) appendDebug(projectRoot, QString("Backup log:\n%1").arg(backupLog));

    if (backupFolder.isEmpty())
    {
        log += QObject::tr("\n[Backup] ERROR: Backup failed.\n");
        log += backupLog;
    }
    else
    {
        log += QObject::tr("\n[Backup] OK: %1\n").arg(backupFolder);
        if (!backupLog.trimmed().isEmpty()) log += backupLog;
    }

    if (outProcessLog) *outProcessLog = log;

    appendDebug(projectRoot, "SUCCESS");
    postStatus(status, QObject::tr("Project‑Source + backup complete."), 5000);
    return outPath;
}

/*!
 * ****************************************************************************************************************************
 * @brief        Generates a tree-style listing of the project directory.
 *
 * @details      Respects include extensions and excluded directories. Output is written
 *               to Project-Tree.txt in the project root. This function is stateless and
 *               uses only the parameters provided by the caller.
 *****************************************************************************************************************************/
QString ProjectSourceExporter::createTree(QWidget *parent, QStatusBar *status, const QString &projectRoot, const QStringList &includeExts, const QStringList &excludeDirs, QString *outLog)
{
    QString log;

    if (projectRoot.trimmed().isEmpty() || !QDir(projectRoot).exists())
    {
        log += QObject::tr("ERROR: Invalid project root: %1\n").arg(projectRoot);
        if (outLog) *outLog = log;
        postStatus(status, QObject::tr("Invalid project root."), 5000);
        return {};
    }

    postStatus(status, QObject::tr("Generating project tree..."));

    const QString outPath = QDir(projectRoot).filePath(QStringLiteral("Project-Tree.txt"));
    QFile outFile(outPath);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        log += QObject::tr("ERROR: Unable to write tree file: %1\n").arg(outPath);
        if (outLog) *outLog = log;
        return {};
    }

    QTextStream ts(&outFile);
    const QDir rootDir(projectRoot);

    /*!
     * ************************************************************************************************************************
     * @brief Recursive aligned tree walker with full ancestor-last tracking.
     *************************************************************************************************************************/
    std::function<void(const QString &, const QVector<bool> &)> walk = [&](const QString &absPath, const QVector<bool> &lastFlags)
    {
        QDir dir(absPath);
        QFileInfoList entries = dir.entryInfoList(
            QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System,
            QDir::DirsFirst | QDir::Name);

        // Filter out excluded entries
        QList<QFileInfo> filtered;
        filtered.reserve(entries.size());

        for (const QFileInfo &fi : std::as_const(entries))
        {
            const QString rel = rootDir.relativeFilePath(fi.absoluteFilePath());
            if (!matchAnyExclude(rel, fi.fileName(), excludeDirs)) filtered.append(fi);
        }

               // Sort: directories first, then files, both alphabetically
        std::sort(filtered.begin(), filtered.end(),
                  [](const QFileInfo &a, const QFileInfo &b)
                  {
                      if (a.isDir() && !b.isDir()) return true;
                      if (!a.isDir() && b.isDir()) return false;
                      return a.fileName().compare(b.fileName(), Qt::CaseInsensitive) < 0;
                  });

        const int count = filtered.size();
        int index = 0;

        for (const QFileInfo &fi : filtered)
        {
            index++;
            const bool isLast = (index == count);

                   // Build indentation from ancestor flags
            QString prefix;
            for (bool ancestorLast : lastFlags)
                prefix += ancestorLast ? QStringLiteral("     ") : QStringLiteral("│    ");

                   // Connector
            QString connector = isLast ? QStringLiteral("└── ") : QStringLiteral("├── ");

            ts << prefix << connector << fi.fileName() << "\n";

            if (fi.isDir())
            {
                QVector<bool> childFlags = lastFlags;
                childFlags.append(isLast);
                walk(fi.absoluteFilePath(), childFlags);
            }
        }
    };

    ts << ".\n";
    walk(projectRoot, {});

    outFile.close();
    postStatus(status, QObject::tr("Project tree generated."), 5000);

    if (outLog) *outLog = log;
    return outPath;
}

/*!
 * ********************************************************************************************************************************
 * @brief   Extracts excluded directory rules from a project's .gitignore file.
 *
 * @details Reads the `.gitignore` file located at the specified project root and
 *          returns a list of directory exclusion rules suitable for directory
 *          filtering. The parser:
 *            - Skips empty lines and comments
 *            - Ignores negation rules (!)
 *            - Accepts only directory rules ending with '/'
 *            - Rejects file-extension and single-file rules
 *            - Normalizes trailing slashes and glob suffixes
 *
 * @param[in] projectRoot Absolute path to the project root directory.
 *
 * @return  A list of relative directory paths to exclude, with no trailing slashes.
 * *******************************************************************************************************************************/
QStringList ProjectSourceExporter::excludeDirsFromGitignore(const QString &projectRoot)
{
    QStringList result;

    const QString gitignorePath =
        QDir(projectRoot).filePath(QStringLiteral(".gitignore"));

    QFile file(gitignorePath);
    if (!file.exists() ||
        !file.open(QIODevice::ReadOnly | QIODevice::Text))
        return result;

    QTextStream in(&file);

    while (!in.atEnd())
    {
        QString line = in.readLine().trimmed();

        // Skip empty lines and comments
        if (line.isEmpty() || line.startsWith('#'))
            continue;

        // Ignore negation rules
        if (line.startsWith('!'))
            continue;

        // ✅ Directory rules ONLY: must end with '/'
        if (!line.endsWith('/'))
            continue;

        // Remove trailing '/'
        line.chop(1);

        // Normalize gitignore-style "dir/*"
        if (line.endsWith("/*"))
            line.chop(2);

        if (!result.contains(line))
            result.append(line);
    }

    return result;
}

/*! ******************************************************************************************************************************
 * @brief        Recursively collects project source files using native Qt APIs.
 *
 * @details      Walks the project directory tree starting at the given root directory,
 *               applying extension filters and gitignore-style exclusion rules.
 *               This function replaces all CMake-based source discovery logic.
 *
 * @param[in]    rootDir             Absolute project root directory.
 * @param[in]    includeExtensions   List of allowed file extensions (without dots).
 * @param[in]    ignoreMatcher       GitIgnoreMatcher instance for exclusion rules.
 *
 * @return       List of relative file paths representing project source files.
 ******************************************************************************************************************************* */
QStringList ProjectSourceExporter::collectSourceFiles(const QString &projectRoot, const QDir &dir, const QStringList &includeExtensions, const GitIgnoreMatcher &ignoreMatcher)
{
    QStringList collectedFiles;

    const QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &entry : entries)
    {
        const QString relativePath = QDir(projectRoot).relativeFilePath(entry.absoluteFilePath());

        if (ignoreMatcher.isIgnored(relativePath)) continue;

        if (entry.isDir())
        {
            collectedFiles.append(collectSourceFiles(projectRoot, QDir(entry.absoluteFilePath()), includeExtensions, ignoreMatcher));
            continue;
        }

        const QString extension = entry.suffix().toLower();
        if (!includeExtensions.contains(extension)) continue;

        collectedFiles.append(relativePath);
    }

    return collectedFiles;
}

/*! ******************************************************************************************************************************
 * @brief        Writes the header section of Project‑Source.txt.
 *
 * @details      Outputs the metadata header for the Project‑Source.txt file, including
 *               the project root path, included file extensions, excluded directory
 *               patterns, and the total number of collected source files. This function
 *               replaces the header‑generation logic previously implemented in
 *               cmake/PackSource.cmake.
 *
 * @param[in]    out                 Open QTextStream used to write the output.
 * @param[in]    projectRoot         Absolute path to the project root directory.
 * @param[in]    includeExtensions   List of included file extensions (without dots).
 * @param[in]    excludedDirs        List of excluded directory patterns.
 * @param[in]    fileCount           Total number of collected source files.
 *
 * @return       true if the header was written successfully; false otherwise.
 ******************************************************************************************************************************* */
bool ProjectSourceExporter::writeProjectSourceHeader(QTextStream &out, const QString &projectRoot, const QStringList &includeExtensions, const QStringList &excludedDirs, int fileCount)
{
    if (!out.device() || !out.device()->isOpen()) return false;

    out << "Project-Source.txt Generated by ProjectSourceExporter\n";
    out << "Root: " << projectRoot << "\n";
    out << "Included extensions: " << includeExtensions.join(';') << "\n";
    out << "Excluded dirs: " << excludedDirs.join(';') << "\n";
    out << "File count: " << fileCount << "\n";
    out << "\n";
    out << "===================================================================================================================================\n";
    out << "==================================== PROJECT TREE ====================================\n";
    out << "===================================================================================================================================\n";

    return true;
}

/*! ******************************************************************************************************************************
 * @brief        Writes the project directory tree to Project‑Source.txt.
 *
 * @details      Recursively traverses the project directory structure and writes a
 *               formatted tree representation to the output stream, using UTF‑8
 *               box‑drawing characters. Directory and file entries are written
 *               relative to the project root, and exclusion rules are applied
 *               consistently to match the behavior previously implemented in
 *               cmake/PackSource.cmake.
 *
 * @param[in]    out             Open QTextStream used to write the output.
 * @param[in]    projectRoot     Absolute path to the project root directory.
 * @param[in]    dir             Directory currently being rendered.
 * @param[in]    ignoreMatcher   GitIgnoreMatcher used to exclude paths.
 * @param[in]    prefix          Current tree prefix used for indentation.
 *
 * @return       void
 ******************************************************************************************************************************* */
void ProjectSourceExporter::writeProjectTree(QTextStream &out, const QString &projectRoot, const QDir &dir, const GitIgnoreMatcher &ignoreMatcher, const QString &prefix)
{
    if (!out.device() || !out.device()->isOpen()) return;

    QFileInfoList entries = dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot, QDir::DirsFirst | QDir::Name);

    for (int i = 0; i < entries.size(); ++i)
    {
        const QFileInfo &entry = entries.at(i);
        const bool isLast = (i == entries.size() - 1);

        const QString relativePath = QDir(projectRoot).relativeFilePath(entry.absoluteFilePath());
        if (ignoreMatcher.isIgnored(relativePath)) continue;

        const QString branch = isLast ? QStringLiteral("└── ") : QStringLiteral("├── ");
        out << prefix << branch << entry.fileName() << "\n";

        if (entry.isDir())
        {
            const QString childPrefix = prefix + (isLast ? QStringLiteral("    ") : QStringLiteral("│   "));
            writeProjectTree(out, projectRoot, QDir(entry.absoluteFilePath()), ignoreMatcher, childPrefix);
        }
    }
}

/*!
 * ********************************************************************************************************************************
 * @file         ProjectSourceExporter.cpp
 * @brief        End of ProjectSourceExporter.cpp.
 *********************************************************************************************************************************/
