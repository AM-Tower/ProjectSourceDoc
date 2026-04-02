/*! ******************************************************************************************************************************
 * @file        ProjectTreeBuilder.h
 * @brief       Project source tree construction utility.
 * @details     Walks the filesystem and builds an in-memory representation of the project
 *              structure using CMake and Git ignore metadata.
 ******************************************************************************************************************************* */
#pragma once

#include "ProjectNode.h"
#include "GitIgnoreMatcher.h"

#include <QSet>
#include <QString>

/*! ******************************************************************************************************************************
 * @class       ProjectTreeBuilder
 * @brief       Builds a runtime project tree model.
 *
 * @public
 ******************************************************************************************************************************* */
class ProjectTreeBuilder
{
public:
    /*! **************************************************************************************************************************
     * @brief       Builds a project tree from the filesystem.
     * @param[in]   projectRoot Absolute project root directory.
     * @param[in]   cmakeSources Set of files explicitly referenced by CMake.
     * @param[in]   gitIgnore Active Git ignore matcher.
     * @return      Pointer to the root ProjectNode.
     *************************************************************************************************************************** */
    static ProjectNode* build(const QString& projectRoot,
                              const QSet<QString>& cmakeSources,
                              const GitIgnoreMatcher& gitIgnore);

private:
    static ProjectNode* walk(const QString& directoryPath,
                             const QSet<QString>& cmakeSources,
                             const GitIgnoreMatcher& gitIgnore);
};