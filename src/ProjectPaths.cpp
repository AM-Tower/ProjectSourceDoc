/*!
 * ********************************************************************************************************************************
 * @file        ProjectPaths.cpp
 * @brief       Implements path utilities for ProjectSource (native + WSL-safe conversion).
 * @details     Implements psd::paths::toBashPath() using wsl.exe wslpath -a on Windows.
 *
 * @authors     Jeffrey Scott Flesher with the help of AI: Copilot
 * @date        2026-03-26
 *********************************************************************************************************************************/

#include "ProjectPaths.h"

#include <QProcess>

namespace psd::paths
{
    /*!
     * ****************************************************************************************************************************
     * @brief   Converts a Windows-native path into a WSL-safe Linux path using wslpath.
     * @param   windowsPath Absolute Windows path (C:\...).
     * @return  WSL absolute path (/mnt/c/...) or empty string on failure.
     *****************************************************************************************************************************/
    static QString windowsPathToWslPath(const QString &windowsPath)
    {
        if (windowsPath.trimmed().isEmpty())
            return QString();

        QString normalized = windowsPath;
        normalized.replace('\\', '/');

        QProcess process;
        process.setProgram(QStringLiteral("wsl.exe"));
        process.setArguments(
            {QStringLiteral("-e"), QStringLiteral("wslpath"), QStringLiteral("-a"), normalized});

        process.start();
        if (!process.waitForStarted(5000))
            return QString();

        process.waitForFinished(-1);

        if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
            return QString();

        return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    }

    /*!
     * ****************************************************************************************************************************
     * @brief   Converts a native filesystem path into a WSL-safe bash path (Windows only).
     * @param   nativePath Absolute native path.
     * @return  Absolute bash path on Windows; returns nativePath unchanged on non-Windows.
     *****************************************************************************************************************************/
    QString toBashPath(const QString &nativePath)
    {
        #if defined(Q_OS_WIN)
        return windowsPathToWslPath(nativePath);
        #else
        return nativePath;
        #endif
    }

} // namespace psd::paths

/*!
 * ********************************************************************************************************************************
 * End of ProjectPaths.cpp
 *********************************************************************************************************************************/
