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
 * @date         2026-03-31
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
#include <QStatusBar>
#include <QTimer>
#include <QUrl>
#include <utility> // for std::as_const

namespace
{
    static constexpr bool kDebugStatus = true;
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

/*!
 * ********************************************************************************************************************************
 * @brief        Generates Project‑Source.txt and returns the output path (settings‑aware).
 *
 * @details      This is the core implementation used by all overloads. It validates the project,
 *               invokes cmake/PackSource.cmake, captures output, and reports progress.
 *********************************************************************************************************************************/
QString ProjectSourceExporter::generateAndReturnPath( QWidget *parent, QStatusBar *status, const QString &projectRoot, const QStringList &includeExts, const QStringList &excludeDirs, QString *outProcessLog)
{
    Q_UNUSED(parent)

    if (projectRoot.trimmed().isEmpty()) return {};

    if (!QDir(projectRoot).exists()) return {};

    QString log;
    QTextStream logStream(&log);

    postStatus(status, QObject::tr("Generating Project‑Source.txt..."));

    /* =====================================================================
     * Invoke PackSource.cmake
     * ===================================================================== */

    QProcess proc;
    proc.setProgram(QStringLiteral("cmake"));

    QStringList args;

    args << QStringLiteral("-DPROJECT_ROOT=%1").arg(projectRoot)
         << QStringLiteral("-DINCLUDE_EXTS=%1").arg(includeExts.join(';'))
         << QStringLiteral("-DEXCLUDE_DIRS=%1").arg(excludeDirs.join(';'))
         << QStringLiteral("-P")
         << QStringLiteral("cmake/PackSource.cmake");

    proc.setArguments(args);
    proc.setWorkingDirectory(projectRoot);

    proc.start();
    if (!proc.waitForFinished(-1))
    {
        log += QObject::tr("ERROR: cmake process did not finish.\n");
        if (outProcessLog) *outProcessLog = log;
        return {};
    }

    log += QString::fromUtf8(proc.readAllStandardOutput());
    log += QString::fromUtf8(proc.readAllStandardError());

    if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0)
    {
        log += QObject::tr("ERROR: Project‑Source generation failed.\n");
        if (outProcessLog) *outProcessLog = log;
        return {};
    }

    const QString outPath = QDir(projectRoot).filePath(QStringLiteral("Project-Source.txt"));

    if (!QFileInfo::exists(outPath))
    {
        log += QObject::tr("ERROR: Project‑Source.txt was not created.\n");
        if (outProcessLog) *outProcessLog = log;
        return {};
    }

    if (outProcessLog) *outProcessLog = log;

    postStatus(status, QObject::tr("Project‑Source.txt generated"), 3000);

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
 * @file         ProjectSourceExporter.cpp
 * @brief        End of ProjectSourceExporter.cpp.
 *********************************************************************************************************************************/
