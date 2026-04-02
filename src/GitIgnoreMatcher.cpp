/*! ******************************************************************************************************************************
 * @file        GitIgnoreMatcher.cpp
 * @brief       Implementation of .gitignore matcher.
 ******************************************************************************************************************************* */

#include "GitIgnoreMatcher.h"

#include <QFile>

GitIgnoreMatcher::GitIgnoreMatcher(const QString& projectRoot)
{
    QFile file(projectRoot + "/.gitignore");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    while (!file.atEnd())
    {
        QString line = QString::fromUtf8(file.readLine()).trimmed();

        if (line.isEmpty() || line.startsWith('#'))
            continue;

        line.replace(".", "\\.");
        line.replace("*", ".*");

        m_patterns.append(QRegularExpression(line));
    }
}

bool GitIgnoreMatcher::isIgnored(const QString& absolutePath) const
{
    for (const auto& pattern : m_patterns)
    {
        if (pattern.match(absolutePath).hasMatch())
            return true;
    }
    return false;
}


/*!
 * ********************************************************************************************************************************
 * End of GitIgnoreMatcher.h
 *********************************************************************************************************************************/