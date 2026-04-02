/*! ******************************************************************************************************************************
 * @file        CMakeSourceParser.cpp
 * @brief       Implementation of runtime CMake source parser.
 * @details     Performs regex-based extraction of source file references from CMakeLists.txt.
 ******************************************************************************************************************************* */

#include "CMakeSourceParser.h"

#include <QFile>
#include <QDir>
#include <QRegularExpression>

QSet<QString> CMakeSourceParser::parseSources(const QString& projectRoot)
{
    QSet<QString> sources;

    QFile file(projectRoot + "/CMakeLists.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return sources;

    const QString contents = QString::fromUtf8(file.readAll());

    static const QRegularExpression sourceRegex(R"(([A-Za-z0-9_\/\.-]+\.(cpp|c|h|hpp|qml|ui|qrc)))");

    auto matchIt = sourceRegex.globalMatch(contents);
    while (matchIt.hasNext())
    {
        const auto match = matchIt.next();
        const QString relativePath = match.captured(1);
        const QString absolutePath = QDir(projectRoot).absoluteFilePath(relativePath);

        sources.insert(QDir::cleanPath(absolutePath));
    }

    return sources;
}

/*!
 * ********************************************************************************************************************************
 * End of CMakeSourceParser.cpp
 *********************************************************************************************************************************/