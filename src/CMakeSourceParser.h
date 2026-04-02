/*! ******************************************************************************************************************************
 * @file        CMakeSourceParser.h
 * @brief       Runtime CMake source file parser.
 * @details     Provides static utilities for parsing CMakeLists.txt files at runtime to determine
 *              which source files are explicitly included by a project. This parser performs
 *              lightweight textual analysis and does not invoke CMake.
 *
 * @note        This component is intentionally heuristic-based and does not attempt to fully
 *              interpret CMake semantics.
 *
 * @author      Jeffrey Scott Flesher
 ******************************************************************************************************************************* */
#pragma once

#include <QString>
#include <QSet>

/*! ******************************************************************************************************************************
 * @class       CMakeSourceParser
 * @brief       Extracts source file references from CMakeLists.txt.
 * @details     Parses common CMake commands such as add_executable(), add_library(),
 *              qt_add_executable(), and target_sources() to collect referenced source files.
 *
 * @public
 ******************************************************************************************************************************* */
class CMakeSourceParser
{
public:
    /*! **************************************************************************************************************************
     * @brief       Parses the root CMakeLists.txt file for source references.
     * @param[in]   projectRoot Absolute path to the project root directory.
     * @return      Set of absolute file paths explicitly referenced by CMake.
     *************************************************************************************************************************** */
    static QSet<QString> parseSources(const QString& projectRoot);
};

/*!
 * ********************************************************************************************************************************
 * End of CMakeSourceParser.h
 *********************************************************************************************************************************/