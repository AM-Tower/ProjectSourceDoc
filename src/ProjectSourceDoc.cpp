/*!
 * ********************************************************************************************************************************
 * @file        ProjectSourceDoc.cpp
 * @brief       Project source document generation facade implementation.
 * @details     Dispatches project source generation to ProjectSourceGenerator.
 * @authors     Jeffrey Scott Flesher with the help of AI: Copilot
 * @date        2026-04-02
 *********************************************************************************************************************************/
#include "ProjectSourceDoc.h"

#include "ProjectSourceGenerator.h"

#include <QDir>

/*!
 * ****************************************************************************************************************************
 * @brief   Construct ProjectSourceDoc.
 * @details Stores the project root directory.
 * @param   projectRoot Absolute project root directory.
 *****************************************************************************************************************************/
ProjectSourceDoc::ProjectSourceDoc(const QString &projectRoot) : m_projectRoot(projectRoot)
{
}

/*!
 * ****************************************************************************************************************************
 * @brief   Generate Project-Source.txt.
 * @details Invokes the C++ ProjectSourceGenerator and bypasses CMake entirely.
 * @return  True on success; false on failure.
 *****************************************************************************************************************************/
bool ProjectSourceDoc::generate()
{
    const QString outFilePath = QDir(m_projectRoot).filePath(QStringLiteral("Project-Source.txt"));

    const QStringList includeExts =
        {
            QStringLiteral("cpp"),
            QStringLiteral("h"),
            QStringLiteral("hpp"),
            QStringLiteral("c"),
            QStringLiteral("cmake"),
            QStringLiteral("md"),
            QStringLiteral("in"),
            QStringLiteral("txt")
        };

    const QStringList excludeDirs =
        {
            QStringLiteral(".git"),
            QStringLiteral("build"),
            QStringLiteral("out"),
            QStringLiteral("cmake")
        };

    return ProjectSourceGenerator::generateProjectSource(m_projectRoot, includeExts, excludeDirs, outFilePath);
}

/*!
 * ********************************************************************************************************************************
 * @brief End of ProjectSourceDoc.cpp
 *********************************************************************************************************************************/
