#!/usr/bin/env bash
# ****************************************************************************************************************************
# @file        install_projectsourcedoc.sh
# @brief       Install or update ProjectSourceDoc AppImage and create desktop entry.
# @details     Searches for the AppImage in ~/Downloads by default, installs it into ~/.local/bin,
#              makes it executable, and creates a .desktop launcher for menu integration.
#              Safe to run multiple times to update the installed version.
# @authors     Jeffrey Scott Flesher with the help of AI: Copilot
# @date        2026-04-04
# clear; chmod +x install.sh && shellcheck install.sh && ./install.sh
# ****************************************************************************************************************************

set -euo pipefail;

log_error()
{
    printf "%b%s%b\n" "\033[1;31m" "Error: $1" "\033[0m";
}

log_info()
{
    printf "%b%s%b\n" "\033[1;34m" "$1" "\033[0m";
}

main()
{
    local APPIMAGE_NAME="ProjectSourceDoc-0.1.1-x86_64.AppImage";
    local DOWNLOAD_PATH="${HOME}/Downloads/${APPIMAGE_NAME}";
    local LOCAL_PATH="./${APPIMAGE_NAME}";
    local SOURCE_FILE="";
    local INSTALL_DIR="${HOME}/.local/bin";
    local DESKTOP_DIR="${HOME}/.local/share/applications";
    local DESKTOP_FILE="${DESKTOP_DIR}/ProjectSourceDoc.desktop";

    # Determine source file location
    if [[ -f "${LOCAL_PATH}" ]]; then SOURCE_FILE="${LOCAL_PATH}";
    elif [[ -f "${DOWNLOAD_PATH}" ]]; then SOURCE_FILE="${DOWNLOAD_PATH}";
    else log_error "${APPIMAGE_NAME} not found in current directory or ~/Downloads."; exit 1; fi

    mkdir -p "${INSTALL_DIR}";
    mkdir -p "${DESKTOP_DIR}";

    # Install or update AppImage
    cp -f "${SOURCE_FILE}" "${INSTALL_DIR}/${APPIMAGE_NAME}";
    chmod +x "${INSTALL_DIR}/${APPIMAGE_NAME}";

    # Create or update desktop entry
    cat > "${DESKTOP_FILE}" <<EOF
[Desktop Entry]
Type=Application
Name=ProjectSourceDoc
Exec=${INSTALL_DIR}/${APPIMAGE_NAME}
Icon=projectsource
Terminal=false
Categories=Utility;Development;
EOF

    if command -v update-desktop-database >/dev/null 2>&1; then update-desktop-database "${DESKTOP_DIR}"; fi

    log_info "Installed/Updated: ${INSTALL_DIR}/${APPIMAGE_NAME}";
    log_info "Menu entry: ${DESKTOP_FILE}";
}

main "$@";

# ****************************************************************************************************************************
# @brief End of install_projectsourcedoc.sh
# ****************************************************************************************************************************
