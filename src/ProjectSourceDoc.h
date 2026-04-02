/*!
 * ********************************************************************************************************************************
 * @file        ProjectSourceDoc.h
 * @brief       Project source document generation facade.
 * @details     Replaces legacy PackSource.cmake invocation with C++ generator orchestration.
 * @authors     Jeffrey Scott Flesher with the help of AI: Copilot
 * @date        2026-04-02
 *********************************************************************************************************************************/
#pragma once

#include <QString>

/*!
 * ****************************************************************************************************************************
 * @class   ProjectSourceDoc
 * @brief   High-level interface for generating Project-Source.txt.
 * @details Owns configuration and dispatches to ProjectSourceGenerator.
 *****************************************************************************************************************************/
class ProjectSourceDoc
{
    public:
        /*!
         * ****************************************************************************************************************************
         * @brief   Construct ProjectSourceDoc.
         * @details Stores the project root directory.
         * @param   projectRoot Absolute project root directory.
         *****************************************************************************************************************************/
        explicit ProjectSourceDoc(const QString &projectRoot);

        /*!
         * ****************************************************************************************************************************
         * @brief   Generate Project-Source.txt.
         * @details Fully replaces PackSource.cmake execution path.
         * @return  True on success; false on failure.
         *****************************************************************************************************************************/
        bool generate();

    private:
        QString m_projectRoot;
};

/*!
 * ********************************************************************************************************************************
 * @brief End of ProjectSourceDoc.h
 *********************************************************************************************************************************/
