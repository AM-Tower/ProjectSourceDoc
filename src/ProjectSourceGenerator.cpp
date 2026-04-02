/*! ******************************************************************************************************************************
 * @file        ProjectSourceGenerator.cpp
 * @brief       Implementation of ProjectSourceGenerator.
 ******************************************************************************************************************************* */

#include "ProjectSourceGenerator.h"

#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QRegularExpression>
#include <QTextStream>

QString ProjectSourceGenerator::generate(const QString& projectRoot)
{
    ensureGitIgnoreExists(projectRoot);

    const QSet<QString> sources = parseCMakeSources(projectRoot);
    const QString tree = buildTree(sources, projectRoot);

    const QString outPath =
        QDir(projectRoot).filePath(QStringLiteral("Project-Source.txt"));

    writeProjectSource(outPath, tree, sources);
    backupFiles(projectRoot, sources);

    return outPath;
}

void ProjectSourceGenerator::ensureGitIgnoreExists(const QString& projectRoot)
{
    QFile gitIgnore(QDir(projectRoot).filePath(".gitignore"));
    if (gitIgnore.exists())
        return;

    gitIgnore.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out(&gitIgnore);

    out << "build/\n";
    out << ".git/\n";
    out << "*.user\n";
    out << "*.autosave\n";
    out << "*.log\n";
}

QSet<QString> ProjectSourceGenerator::parseCMakeSources(const QString& projectRoot)
{
    QSet<QString> result;
    QFile cmake(QDir(projectRoot).filePath("CMakeLists.txt"));

    if (!cmake.open(QIODevice::ReadOnly | QIODevice::Text))
        return result;

    const QString text = QString::fromUtf8(cmake.readAll());

    QRegularExpression re(R"(([A-Za-z0-9_\/\.-]+\.(cpp|c|h|hpp|qml|ui|qrc|txt|md)))");
    auto it = re.globalMatch(text);

    while (it.hasNext())
    {
        const QString rel = it.next().captured(1);
        result.insert(QDir(projectRoot).absoluteFilePath(rel));
    }

    return result;
}

QString ProjectSourceGenerator::buildTree(const QSet<QString>& files, const QString& root)
{
    QStringList lines;
    lines << "PROJECT TREE:";
    lines << "------------------------------------------------------------";

    for (const QString& file : files)
        lines << QDir(root).relativeFilePath(file);

    return lines.join('\n');
}

void ProjectSourceGenerator::writeProjectSource(const QString& outPath, const QString& tree, const QSet<QString>& files)
{
    QFile out(outPath);
    out.open(QIODevice::WriteOnly | QIODevice::Text);

    QTextStream stream(&out);

    stream << tree << "\n\n";

    for (const QString& filePath : files)
    {
        QFile f(filePath);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;

        stream << "=============================================\n";
        stream << "FILE: " << filePath << "\n";
        stream << "=============================================\n";
        stream << QString::fromUtf8(f.readAll()) << "\n\n";
    }
}

void ProjectSourceGenerator::backupFiles(const QString& projectRoot, const QSet<QString>& files)
{
    const QString backupDir = QDir(projectRoot).filePath("backup/" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));

    QDir().mkpath(backupDir);

    for (const QString& file : files)
    {
        const QString rel = QDir(projectRoot).relativeFilePath(file);
        const QString dst = QDir(backupDir).filePath(rel);

        QDir().mkpath(QFileInfo(dst).path());
        QFile::copy(file, dst);
    }
}

