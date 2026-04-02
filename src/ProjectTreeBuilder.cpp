/*!
 * ********************************************************************************************************************************
 * @file        ProjectTreeBuilder.cpp
 * @brief       Deterministic project tree construction implementation.
 * @details     Emits a directory-first project tree with correct sibling-aware indentation.
 * @authors     Jeffrey Scott Flesher with the help of AI: Copilot
 * @date        2026-04-02
 *********************************************************************************************************************************/
#include "ProjectTreeBuilder.h"

#include <QDir>
#include <QFileInfo>
#include <QFileInfoList>

static const QString kTreeDivider =
    QStringLiteral("===================================================================================================================================");

/*!
 * ****************************************************************************************************************************
 * @brief   Build the formatted project tree text.
 * @details Initializes sibling tracking and dispatches recursion.
 * @param   projectRoot Absolute project root directory.
 * @param   excludeDirs Directory exclusion patterns.
 * @return  Fully formatted project tree text.
 *****************************************************************************************************************************/
QString ProjectTreeBuilder::buildProjectTreeText(const QString &projectRoot, const QStringList &excludeDirs)
{
    QStringList lines;

    lines.append(kTreeDivider);
    lines.append(QStringLiteral("==================================== PROJECT TREE ===================================="));
    lines.append(kTreeDivider);
    lines.append(QStringLiteral("."));

    QVector<bool> continueAtDepth;
    walkDirectory(projectRoot, QString(), excludeDirs, lines, continueAtDepth);

    lines.append(QString());

    return lines.join(QLatin1Char('\n'));
}

/*!
 * ****************************************************************************************************************************
 * @brief   Recursively walk directories and emit tree entries.
 * @details Excludes directories only at root level and renders correct indentation.
 * @param   projectRoot Absolute project root directory.
 * @param   relativePath Current relative path.
 * @param   excludeDirs Directory exclusion patterns.
 * @param   outLines Accumulated output lines.
 * @param   continueAtDepth Tracks whether to draw vertical bars per depth.
 *****************************************************************************************************************************/
void ProjectTreeBuilder::walkDirectory(const QString &projectRoot, const QString &relativePath, const QStringList &excludeDirs, QStringList &outLines, QVector<bool> &continueAtDepth)
{
    const QString absolutePath = relativePath.isEmpty() ? projectRoot : projectRoot + QLatin1Char('/') + relativePath;
    const QDir dir(absolutePath);

    const QFileInfoList dirEntries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    const QFileInfoList fileEntries = dir.entryInfoList(QDir::Files, QDir::Name);

    QFileInfoList filteredDirs;

    for (int i = 0; i < dirEntries.size(); ++i)
    {
        const QFileInfo &info = dirEntries.at(i);

        if (continueAtDepth.isEmpty())
        {
            const QString name = info.fileName();
            bool excluded = false;

            for (int p = 0; p < excludeDirs.size(); ++p)
            {
                const QString &pattern = excludeDirs.at(p);
                if (pattern.endsWith('*') && name.startsWith(pattern.left(pattern.size() - 1))) { excluded = true; break; }
                if (!pattern.endsWith('*') && name == pattern) { excluded = true; break; }
            }

            if (excluded) continue;
        }

        filteredDirs.append(info);
    }

    const int totalEntries = filteredDirs.size() + fileEntries.size();
    int index = 0;

    auto buildIndent = [&continueAtDepth]() -> QString
    {
        QString indent;
        for (int i = 0; i < continueAtDepth.size(); ++i)
            indent += continueAtDepth.at(i) ? QStringLiteral("│   ") : QStringLiteral("    ");
        return indent;
    };

    for (int i = 0; i < filteredDirs.size(); ++i)
    {
        const QFileInfo &info = filteredDirs.at(i);
        const bool isLast = (++index == totalEntries);

        outLines.append(buildIndent() + (isLast ? QStringLiteral("└── ") : QStringLiteral("├── ")) + info.fileName());

        continueAtDepth.append(!isLast);

        const QString nextRelativePath = relativePath.isEmpty() ? info.fileName() : relativePath + QLatin1Char('/') + info.fileName();
        walkDirectory(projectRoot, nextRelativePath, excludeDirs, outLines, continueAtDepth);

        continueAtDepth.removeLast();
    }

    for (int i = 0; i < fileEntries.size(); ++i)
    {
        const QFileInfo &info = fileEntries.at(i);
        const bool isLast = (++index == totalEntries);

        outLines.append(buildIndent() + (isLast ? QStringLiteral("└── ") : QStringLiteral("├── ")) + info.fileName());
    }
}

/*!
 * ********************************************************************************************************************************
 * @brief End of ProjectTreeBuilder.cpp
 *********************************************************************************************************************************/
