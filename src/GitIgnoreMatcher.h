/*! ******************************************************************************************************************************
 * @file        GitIgnoreMatcher.h
 * @brief       Runtime .gitignore rule matcher.
 * @details     Loads and applies basic .gitignore-style ignore patterns during filesystem traversal.
 *
 * @note        This matcher supports simple wildcard rules and is intended for UI filtering,
 *              not for strict Git parity.
 ******************************************************************************************************************************* */
#pragma once

#include <QString>
#include <QList>
#include <QRegularExpression>

/*! ******************************************************************************************************************************
 * @class       GitIgnoreMatcher
 * @brief       Determines whether paths should be ignored based on .gitignore rules.
 * @details     Converts .gitignore entries into regular expressions and evaluates paths against them.
 *
 * @public
 ******************************************************************************************************************************* */
class GitIgnoreMatcher
{
public:
    /*! **************************************************************************************************************************
     * @brief       Constructs a matcher from a project root.
     * @param[in]   projectRoot Absolute path to the project root directory.
     *************************************************************************************************************************** */
    explicit GitIgnoreMatcher(const QString& projectRoot);

    /*! **************************************************************************************************************************
     * @brief       Tests whether a path should be ignored.
     * @param[in]   absolutePath Absolute path to test.
     * @return      True if the path matches an ignore rule.
     *************************************************************************************************************************** */
    bool isIgnored(const QString& absolutePath) const;

private:
    QList<QRegularExpression> m_patterns;
};

/*!
 * ********************************************************************************************************************************
 * End of GitIgnoreMatcher.h
 *********************************************************************************************************************************/