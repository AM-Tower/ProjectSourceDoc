/*!
 * ********************************************************************************************************************************
 * @file        ProjectPaths.h
 * @brief       Path utilities for ProjectSource (native + WSL-safe conversion).
 * @details     These helpers provide minimal path conversion support required by ProjectSource:
 *              - Convert an arbitrary native path to a WSL-safe bash path on Windows.
 *              - On non-Windows platforms, conversion is a no-op.
 *
 *              The ProjectSource application uses an explicitly opened project folder, so these
 * functions do not attempt to discover or cache a “project root”.
 *
 * @authors     Jeffrey Scott Flesher with the help of AI: Copilot
 * @date        2026-03-31
 *********************************************************************************************************************************/
#pragma once

#include <QString>

namespace psd::paths
{
    /*!
     * ****************************************************************************************************************************
     * @brief   Converts a native filesystem path into a WSL-safe bash path (Windows only).
     * @param   nativePath Absolute native path (C:\...).
     * @return  Absolute bash path (/mnt/c/...) on Windows; returns nativePath unchanged on
     * non-Windows.
     *****************************************************************************************************************************/
    QString toBashPath(const QString &nativePath);

} // namespace psd::paths

/*!
 * ********************************************************************************************************************************
 * End of ProjectPaths.h
 *********************************************************************************************************************************/
