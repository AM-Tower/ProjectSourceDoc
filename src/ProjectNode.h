/*! ******************************************************************************************************************************
 * @file        ProjectNode.h
 * @brief       In-memory project tree node.
 * @details     Represents a directory or file within a project source tree, including
 *              metadata related to CMake inclusion and Git ignore state.
 ******************************************************************************************************************************* */
#pragma once

#include <QString>
#include <QList>

/*! ******************************************************************************************************************************
 * @struct      ProjectNode
 * @brief       Represents a node in the project tree.
 *
 * @public
 ******************************************************************************************************************************* */
struct ProjectNode
{
    QString             name;               //!< Display name
    QString             absolutePath;        //!< Absolute filesystem path
    bool                includedByCMake;     //!< Explicitly referenced by CMake
    bool                ignoredByGit;        //!< Excluded via .gitignore
    QList<ProjectNode*> children;            //!< Child nodes
};