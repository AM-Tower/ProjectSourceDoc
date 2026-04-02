/*! ******************************************************************************************************************************
 * @file        ProjectSourceGenerator.h
 * @brief       CMake-driven project source generator.
 * @details     Generates a Project-Source.txt file by reading CMakeLists.txt to determine included
 *              files, applying .gitignore rules for exclusion, printing a tree representation,
 *              copying file contents, and backing up all included files.
 ******************************************************************************************************************************* */
#pragma once

#include <QString>
#include <QStringList>
#include <QSet>

/*! ******************************************************************************************************************************
 * @class       ProjectSourceGenerator
 * @brief       Orchestrates Project-Source.txt generation.
 *
 * @public
 ******************************************************************************************************************************* */
class ProjectSourceGenerator
{
public:
    /*! **************************************************************************************************************************
     * @brief       Entry point for project source generation.
     * @param[in]   projectRoot Absolute path to the CMake project root.
     * @return      Absolute path to the generated Project-Source.txt file.
     *************************************************************************************************************************** */
    static QString generate(const QString& projectRoot);

private:
    static void ensureGitIgnoreExists(const QString& projectRoot);
    static QSet<QString> parseCMakeSources(const QString& projectRoot);
    static QStringList loadGitIgnorePatterns(const QString& projectRoot);
    static QString buildTree(const QSet<QString>& files, const QString& root);
    static void writeProjectSource(const QString& outPath, const QString& tree, const QSet<QString>& files);
    static void backupFiles(const QString& projectRoot, const QSet<QString>& files);
};

