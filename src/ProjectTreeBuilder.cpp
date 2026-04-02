/*! ******************************************************************************************************************************
 * @file        ProjectTreeBuilder.cpp
 * @brief       Implementation of project tree builder.
 ******************************************************************************************************************************* */

#include "ProjectTreeBuilder.h"

#include <QDir>
#include <QDirIterator>

ProjectNode* ProjectTreeBuilder::build(const QString& projectRoot,
                                       const QSet<QString>& cmakeSources,
                                       const GitIgnoreMatcher& gitIgnore)
{
    return walk(projectRoot, cmakeSources, gitIgnore);
}

ProjectNode* ProjectTreeBuilder::walk(const QString& directoryPath,
                                      const QSet<QString>& cmakeSources,
                                      const GitIgnoreMatcher& gitIgnore)
{
    auto* node = new ProjectNode;
    node->name = QFileInfo(directoryPath).fileName();
    node->absolutePath = directoryPath;
    node->includedByCMake = false;
    node->ignoredByGit = false;

    QDirIterator it(directoryPath,
                    QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);

    while (it.hasNext())
    {
        it.next();
        const QString path = it.filePath();

        if (gitIgnore.isIgnored(path))
            continue;

        if (it.fileInfo().isDir())
        {
            node->children.append(walk(path, cmakeSources, gitIgnore));
        }
        else
        {
            auto* fileNode = new ProjectNode;
            fileNode->name = it.fileName();
            fileNode->absolutePath = path;
            fileNode->includedByCMake = cmakeSources.contains(path);
            fileNode->ignoredByGit = false;
            node->children.append(fileNode);
        }
    }

    return node;
}