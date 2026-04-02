/*!
 * ********************************************************************************************************************************
 * @file        ProjectSourceGenerator.h
 * @brief       Project source aggregation and output generation.
 * @details     Declares functionality for generating Project-Source.txt, including project tree output.
 * @authors     Jeffrey Scott Flesher with the help of AI: Copilot
 * @date        2026-04-02
 *********************************************************************************************************************************/
#pragma once

#include <QString>
#include <QStringList>

class QTextStream;

/*!
 * ****************************************************************************************************************************
 * @class   ProjectSourceGenerator
 * @brief   Generates Project-Source.txt for a project.
 * @details Coordinates writing headers, project tree output, and source file contents
 *          into a single aggregated text file.
 *****************************************************************************************************************************/
class ProjectSourceGenerator
{
    public:
        /*!
         * ****************************************************************************************************************************
         * @brief   Generate Project-Source.txt for a project.
         * @details Writes metadata headers, project tree, and source file contents
         *          into the specified output file.
         * @param   projectRoot Absolute project root directory.
         * @param   includeExts File extensions to include.
         * @param   excludeDirs Directory exclusion patterns.
         * @param   outFilePath Absolute output file path.
         * @return  True on success; false on failure.
         *****************************************************************************************************************************/
        static bool generateProjectSource(const QString &projectRoot, const QStringList &includeExts, const QStringList &excludeDirs, const QString &outFilePath);

    private:
        /*!
         * ****************************************************************************************************************************
         * @brief   Write the formatted project tree to Project-Source.txt.
         * @details Replaces the former PackSource.cmake psd_write_project_tree() behavior.
         * @param   out QTextStream bound to Project-Source.txt.
         * @param   projectRoot Absolute project root directory.
         * @param   excludeDirs Directory exclusion patterns.
         *****************************************************************************************************************************/
        static void writeProjectTree(QTextStream &out, const QString &projectRoot, const QStringList &excludeDirs);
};

/*!
 * ********************************************************************************************************************************
 * @brief End of ProjectSourceGenerator.h
 *********************************************************************************************************************************/
