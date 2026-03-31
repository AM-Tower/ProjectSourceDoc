#!/usr/bin/env bash
#**************************************************************************************************
# @file    source_files.sh
# @brief   Central registry of project source files (Bash + metadata).
# @author Jeffrey Scott Flesher with the help of AI: Copilot
# @details
#   - SAFE to source.
#   - Defines arrays and path globals.
#   - Prints a single "Loaded" line once (optional color if defined).
# @date Version: SEE BELOW CODE SCRIPT_VERSION_SOURCE_FILES
# @version Version: SEE BELOW CODE SCRIPT_DATE_SOURCE_FILES
#****************************************************************
declare -gx SCRIPT_VERSION_SOURCE_FILES="0.0.5";
declare -gx SCRIPT_DATE_SOURCE_FILES="[2026-03-11 @ 0800]";

# Bash root (libs + entrypoints live here)
if [[ -z "${PATH_ROOT:-}" ]]; then declare -gx PATH_ROOT; PATH_ROOT="$(cd -- "${PWD}" && pwd -P)"; fi
if [[ -z "${PATH_BASH:-}" ]]; then declare -gx PATH_BASH="${PATH_ROOT}/bash"; fi

declare -gAx ELAPSED_TIMES_LAST=(); # associative array to track elapsed times
declare -gx SCRIPT_START; SCRIPT_START=$(date +%s);

#**************************************************************************************************
# @section Source files (SAFE to source or check)
#**************************************************************************************************

#****************************************************************
# @brief *.sh: Shared libraries designed to be sourced first
#****************************************************************
# shellcheck disable=SC2034
declare -ag SOURCE_SH_LIBRARY=(
    "${PATH_BASH}/library.sh"
);

#****************************************************************
# @brief *.sh: Shared libraries designed to be sourced
# Note that order does matter:
# project-paths.sh, python.sh
#****************************************************************
# shellcheck disable=SC2034
declare -ag SOURCE_SH_SHARED=(
    "${PATH_BASH}/backup.sh"
    "${PATH_BASH}/git.sh"
);

#****************************************************************
# @brief Exclude Folders
#****************************************************************
declare -agx EXCLUDE_FOLDERS=(
    ".git"
    "AppDir"
    "build"
    "build_release"
    "Archive"
    "Backups"
)

#****************************************************************
# @brief Include Folders
#****************************************************************
declare -agx INCLUDE_FOLDERS=(
    "cmake"
    "i18n"
    "resources"
    "src"
    ".vscode"
)

#****************************************************************
# @brief Include file Extensions
#****************************************************************
declare -agx INCLUDE_EXTS=(
    "sh"
    "py"
    "txt"
    "json"
    "md"
    "ps1"
    "in"
    "png"
    "ini"
    "db"
    ".pylintrc"
)

#**************************************************************************************************
# @brief Show loading
#**************************************************************************************************
show_loading_source_files()
{
    printf "%b %s [%s ∅ 00:00:00] Loaded: source_files.sh version (%s) %s%b\n" "${CYAN}" "ℹ" "$(date '+%H:%M:%S')" "${SCRIPT_VERSION_SOURCE_FILES}" "${SCRIPT_DATE_SOURCE_FILES}" "${NC}";
    return 0; # Pass
}

#**************************************************************************************************

show_loading_source_files;

#**************************************************************************************************
# @footer End of source_files.sh
#**************************************************************************************************
