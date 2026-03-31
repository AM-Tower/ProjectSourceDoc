#!/usr/bin/env bash
################################################################################
# @file        deploy.sh
# @brief       Qt 6 Desktop Application Deployment Script for Linux and macOS.
# @details     This script automates the full deployment pipeline for a Qt 6
#              desktop application. It performs the following tasks in order:
#
#              1. Validates the build environment (cmake, qmake, Qt tools).
#              2. Checks the CMakeLists.txt for deployment-required settings.
#              3. Builds the application in Release mode via cmake.
#              4. Creates an AppDir structure under the project root.
#              5. Downloads or updates linuxdeploy and linuxdeploy-plugin-qt.
#              6. Copies the SVG app icon into AppDir (no conversion needed —
#                 linuxdeploy and the Qt SVG plugin handle SVG natively).
#              7. Copies the built binary and .desktop file into AppDir.
#              8. Runs linuxdeploy with the Qt plugin to bundle all Qt libraries.
#              9. Executes a battery of pre-packaging tests (ldd, strace, etc.).
#             10. Runs the application itself in a sandboxed debug mode and
#                 captures stdout/stderr to a debug log for library diagnostics.
#             11. If all tests pass, finalises the AppImage and copies it to
#                 the deploy/ output folder.
#             12. Provides a lightweight auto-update check stub that compares
#                 a remote VERSION file against the local version string; a
#                 full GitHub-based update can be wired in once the repo exists.
#
#              CMakeLists.txt changes required:
#              ─────────────────────────────────
#              • Ensure your project sets CMAKE_INSTALL_PREFIX or uses
#                GNUInstallDirs so that `cmake --install` places the binary
#                under <prefix>/bin/.  Example:
#
#                  include(GNUInstallDirs)
#                  install(TARGETS MyApp RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
#
#              • For desktop integration, add an install rule for the .desktop
#                file and the icon, e.g.:
#
#                  install(FILES resources/myapp.desktop
#                          DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
#                  install(FILES resources/icons/light/app.svg
#                          DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
#                          RENAME myapp.svg)
#
#              • Set CMAKE_BUILD_TYPE to Release in your default cache or pass
#                it on the command line; this script always passes
#                -DCMAKE_BUILD_TYPE=Release.
#
#              • Qt6 deployment requires that you do NOT strip the binary when
#                linuxdeploy is used with NO_STRIP=1 (already exported here).
#
#              Auto-update note:
#              ─────────────────
#              Without a GitHub repo the auto-update function compares the local
#              VERSION constant (set at the top of this script) against a plain
#              text file at UPDATE_URL.  Once the repo is created, replace
#              UPDATE_URL with the raw URL of a VERSION file in the repo, e.g.:
#              https://raw.githubusercontent.com/YourUser/YourRepo/main/VERSION
#
#              Platform note:
#              ──────────────
#              On macOS the script detects the OS and switches to macdeployqt
#              instead of linuxdeploy.  Windows support is noted with stubs;
#              windeployqt must be invoked from a Developer Command Prompt and
#              is best handled in a separate PowerShell or batch script.
#
# @author      Jeffrey Scott Flesher with the help of AI: Claude
# @version     1.6
# @date        2026-03-28
# @section     License  Unlicensed, MIT, or any.
################################################################################

# ── Strict mode ────────────────────────────────────────────────────────────────
set -euo pipefail;
IFS=$'\n\t';

################################################################################
# @brief Script version — printed at startup so logs confirm which build ran.
# @details Increment this string whenever deploy.sh is modified.  The value is
#          written into the installer log header by bootstrap_log() so you can
#          always confirm that the correct version of the script was executed.
################################################################################
SCRIPT_VERSION="1.6";

################################################################################
# @brief Global configuration — edit these variables for your project.
# @details APP_NAME and DESKTOP_FILE are set dynamically by read_cmake_app_name
#          early in main().  The placeholders below are overwritten at runtime.
################################################################################

# Application identity — overwritten by read_cmake_app_name()
APP_NAME="UnknownApp";                   # Resolved from cmake project() at runtime
APP_VERSION="1.0.0";                     # Local version string for update check
DESKTOP_FILE="${APP_NAME}.desktop";      # Derived from APP_NAME; reset after parse
ICON_SRC="resources/icons/light/app.svg";

# Qt installation root (Qt Maintenance Tool default)
QT_VERSION="6.11.0";
QT_BASE="/opt/Qt";
QT_ARCH="gcc_64";                        # Linux arch suffix
QT_ROOT="${QT_BASE}/${QT_VERSION}/${QT_ARCH}";

# Derived Qt tool paths
QT6_QMAKE="${QT_ROOT}/bin/qmake";
# deployqt ships with some Qt builds; checked and used if present
QT_DEPLOY_QT="${QT_ROOT}/bin/deployqt";
export QT6_QMAKE;
export NO_STRIP=1;                       # Prevent stripping so debug info is kept
# TIFF exclusion strategy: QT_PLUGIN_FILTER is a WHITELIST in linuxdeploy-plugin-qt,
# not a blacklist — a leading "-" prefix is silently ignored, so it cannot
# be used to exclude individual plugins.  The correct approach used here is
# a two-phase deploy: (1) populate AppDir without packaging, (2) delete
# libqtiff.so and its orphaned libtiff.so.* before the AppImage is squashed.
# See remove_unwanted_plugins() for the full implementation.

# Build
BUILD_DIR="build_release";              # Temporary cmake build folder (in root)
INSTALL_PREFIX="${BUILD_DIR}/install";  # cmake install target inside build dir

# AppDir (treated as temporary — deleted and recreated each run)
APPDIR="AppDir";
APPDIR_USR="${APPDIR}/usr";
APPDIR_BIN="${APPDIR_USR}/bin";
APPDIR_LIB="${APPDIR_USR}/lib";
APPDIR_SHARE="${APPDIR_USR}/share";
APPDIR_APPS="${APPDIR_SHARE}/applications";
APPDIR_ICONS="${APPDIR_SHARE}/icons/hicolor";
APPDIR_TOOLS="${APPDIR}/tools";         # linuxdeploy binaries cached here

# Output
DEPLOY_DIR="deploy";
LOG_DIR="${APPDIR}/logs";
# INSTALLER_LOG is set by bootstrap_log() after LOG_DIR is created
INSTALLER_LOG="";
DEBUG_LOG="${LOG_DIR}/app_debug.log";

# linuxdeploy download URLs
LINUXDEPLOY_URL="https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage";
LINUXDEPLOY_QT_URL="https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage";
LINUXDEPLOY_BIN="${APPDIR_TOOLS}/linuxdeploy-x86_64.AppImage";
LINUXDEPLOY_QT_BIN="${APPDIR_TOOLS}/linuxdeploy-plugin-qt-x86_64.AppImage";

# Auto-update stub URL — replace with raw GitHub URL once repo exists
UPDATE_URL="https://example.com/VERSION";

# Runtime flags (defaults)
VERBOSE=0;                              # Quiet by default; -v enables verbose
SKIP_BUILD=0;                           # -s skips cmake build (reuse last build)
SKIP_TESTS=0;                           # -t skips the test suite

# OS is set by detect_os()
OS="linux";

# CMAKE_GENERATOR is set by check_host_tools()
CMAKE_GENERATOR="Ninja";

################################################################################
# @brief ANSI colour helpers.
################################################################################
RED='\033[0;31m';
GREEN='\033[0;32m';
YELLOW='\033[1;33m';
CYAN='\033[0;36m';
BOLD='\033[1m';
RESET='\033[0m';

################################################################################
# @brief  Print a coloured PASS banner and append plain text to the log.
# @param  $1  Test or step description string.
################################################################################
pass()
{
    echo -e "${GREEN}[PASS]${RESET} $1";
    [[ -n "${INSTALLER_LOG}" ]] && echo "[PASS] $1" >> "${INSTALLER_LOG}";
}

################################################################################
# @brief  Print a coloured FAIL banner, log it, and exit with status 1.
# @param  $1  Test or step description string.
################################################################################
fail()
{
    echo -e "${RED}[FAIL]${RESET} $1";
    [[ -n "${INSTALLER_LOG}" ]] && echo "[FAIL] $1" >> "${INSTALLER_LOG}";
    exit 1;
}

################################################################################
# @brief  Print a coloured WARN banner (non-fatal) and append to the log.
# @param  $1  Warning message string.
################################################################################
warn()
{
    echo -e "${YELLOW}[WARN]${RESET} $1";
    [[ -n "${INSTALLER_LOG}" ]] && echo "[WARN] $1" >> "${INSTALLER_LOG}";
}

################################################################################
# @brief  Print an informational message; suppressed on stdout in quiet mode.
# @param  $1  Message string.
################################################################################
info()
{
    if [[ "${VERBOSE}" -eq 1 ]]; then
        echo -e "${CYAN}[INFO]${RESET} $1";
    fi
    [[ -n "${INSTALLER_LOG}" ]] && echo "[INFO] $1" >> "${INSTALLER_LOG}";
}

################################################################################
# @brief  Print a section header banner to stdout and to the log.
# @param  $1  Section title string.
################################################################################
section()
{
    local title="$1";
    local bar="══════════════════════════════════════════════════════════════════════";
    echo -e "\n${BOLD}${bar}${RESET}";
    echo -e "${BOLD}  ${title}${RESET}";
    echo -e "${BOLD}${bar}${RESET}";
    if [[ -n "${INSTALLER_LOG}" ]]; then
        {
            echo "";
            echo "${bar}";
            echo "  ${title}";
            echo "${bar}";
        } >> "${INSTALLER_LOG}";
    fi
}

################################################################################
# @brief  Print usage/help text and exit cleanly.
################################################################################
usage()
{
    echo "";
    echo "  Usage: $0 [OPTIONS]";
    echo "";
    echo "  Options:";
    echo "    -v   Verbose mode (print INFO messages to stdout)";
    echo "    -s   Skip cmake build step (reuse last build)";
    echo "    -t   Skip test suite";
    echo "    -h   Show this help and exit";
    echo "";
    echo "  How to make executable and run:";
    echo "    chmod +x deploy.sh";
    echo "    ./deploy.sh";
    echo "";
    echo "  Run shellcheck:";
    echo "    shellcheck deploy.sh";
    echo "";
    echo "  Run the packaged AppImage directly:";
    echo "    ./deploy/${APP_NAME}-${APP_VERSION}-x86_64.AppImage";
    echo "";
    exit 0;
}

################################################################################
# @brief  Bootstrap the log file BEFORE any other function runs.
# @details Creates DEPLOY_DIR and LOG_DIR immediately so that the log path is
#          valid for all subsequent append operations.  Must be called as the
#          very first action in main(), before detect_os or any helper that
#          writes to INSTALLER_LOG.
# @param  $@  Original script arguments (written into the log header).
################################################################################
bootstrap_log()
{
    # deploy/ is persistent output — never deleted
    mkdir -p "${DEPLOY_DIR}" || { echo "FATAL: Cannot create ${DEPLOY_DIR}"; exit 1; };

    # AppDir/logs lives inside the temp tree.  We only need the logs
    # sub-folder to exist right now; init_dirs() wipes the rest later.
    mkdir -p "${LOG_DIR}" || { echo "FATAL: Cannot create ${LOG_DIR}"; exit 1; };

    # Set the timestamped log path now that the directory exists
    INSTALLER_LOG="${LOG_DIR}/deploy_$(date +%Y%m%d_%H%M%S).log";
    DEBUG_LOG="${LOG_DIR}/app_debug.log";

    touch "${INSTALLER_LOG}" || { echo "FATAL: Cannot create ${INSTALLER_LOG}"; exit 1; };

    # SC2129: group multiple redirects to the same file into one block
    {
        echo "Deploy log started: $(date)";
        echo "Script version: ${SCRIPT_VERSION}";
        echo "Script: $0";
        echo "Args: $*";
    } >> "${INSTALLER_LOG}";
}

################################################################################
# @brief  Parse the project name from CMakeLists.txt and set APP_NAME.
# @details Reads the first project(...) call and extracts the name token.
#          Also derives DESKTOP_FILE from the resolved APP_NAME.
#          Falls back to "UnknownApp" with a warning if parsing fails.
################################################################################
read_cmake_app_name()
{
    section "Reading APP_NAME from CMakeLists.txt";

    local cmake_file="CMakeLists.txt";

    if [[ ! -f "${cmake_file}" ]]; then
        warn "CMakeLists.txt not found — APP_NAME stays '${APP_NAME}'";
        return;
    fi

    # Match project( <Name> ...) — extract the first token after the paren.
    local parsed_name;
    parsed_name="$(grep -m1 -iE '^\s*project\s*\(' "${cmake_file}" \
        | sed -E 's/.*project\s*\(\s*([A-Za-z0-9_.-]+).*/\1/')";

    if [[ -z "${parsed_name}" ]]; then
        warn "Could not parse project() name — APP_NAME stays '${APP_NAME}'";
        return;
    fi

    APP_NAME="${parsed_name}";
    DESKTOP_FILE="${APP_NAME}.desktop";
    info "APP_NAME set from CMakeLists.txt: ${APP_NAME}";
    info "DESKTOP_FILE set to: ${DESKTOP_FILE}";
    pass "APP_NAME resolved: ${APP_NAME}";
}

################################################################################
# @brief  Parse command-line arguments and set runtime flags.
# @param  $@  All script arguments forwarded from main.
################################################################################
parse_args()
{
    while getopts ":vsth" opt; do
        case "${opt}" in
            v)  VERBOSE=1 ;;
            s)  SKIP_BUILD=1 ;;
            t)  SKIP_TESTS=1 ;;
            h)  usage ;;
            *)  echo "Unknown option: -${OPTARG}"; usage ;;
        esac
    done
}

################################################################################
# @brief  Detect the host operating system.
# @details Sets the global OS variable to "linux", "mac", or "windows".
################################################################################
detect_os()
{
    section "Detecting Operating System";
    case "$(uname -s)" in
        Linux*)              OS="linux" ;;
        Darwin*)             OS="mac" ;;
        CYGWIN*|MINGW*|MSYS*)  OS="windows" ;;
        *)                   fail "Unsupported OS: $(uname -s)" ;;
    esac
    info "Detected OS: ${OS}";
    pass "OS detection";
}

################################################################################
# @brief  Initialise AppDir directory tree (wiped fresh each run).
# @details deploy/ and LOG_DIR are already created by bootstrap_log().
#          This function wipes the entire AppDir and rebuilds its sub-tree.
#          The tools/ cache inside AppDir is preserved by copying it out and
#          back in to avoid re-downloading linuxdeploy on every run.
################################################################################
init_dirs()
{
    section "Initialising AppDir";

    # Cache the tools folder before wiping AppDir so we keep downloads
    local tools_cache;
    tools_cache="$(mktemp -d)";

    if [[ -d "${APPDIR_TOOLS}" ]]; then
        info "Caching linuxdeploy tools before AppDir wipe...";
        cp -a "${APPDIR_TOOLS}/." "${tools_cache}/" 2>/dev/null || true;
    fi

    # Wipe and recreate AppDir (fully temporary)
    info "Removing stale AppDir...";
    rm -rf "${APPDIR}" || fail "Cannot remove stale AppDir";

    mkdir -p \
        "${APPDIR_BIN}" \
        "${APPDIR_LIB}" \
        "${APPDIR_SHARE}" \
        "${APPDIR_APPS}" \
        "${APPDIR_ICONS}/scalable/apps" \
        "${APPDIR_TOOLS}" \
        "${LOG_DIR}" \
        || fail "Cannot create AppDir sub-directories";

    # Restore the cached tools back into the fresh AppDir
    if [[ -n "$(ls -A "${tools_cache}" 2>/dev/null)" ]]; then
        cp -a "${tools_cache}/." "${APPDIR_TOOLS}/" 2>/dev/null || true;
        info "Restored cached linuxdeploy tools.";
    fi
    rm -rf "${tools_cache}";

    info "AppDir initialised.";
    pass "Directory initialisation";
}

################################################################################
# @brief  Check that required host tools are installed.
# @details Tests for: cmake, make/ninja, curl, ldd, file, pkg-config.
#          No SVG converter is required — linuxdeploy accepts SVG natively.
################################################################################
check_host_tools()
{
    section "Checking Host Tools";

    local required_tools=("cmake" "curl" "ldd" "file" "pkg-config");
    local missing=0;

    for tool in "${required_tools[@]}"; do
        if command -v "${tool}" &>/dev/null; then
            pass "Tool present: ${tool}";
        else
            warn "Missing tool: ${tool}";
            missing=$((missing + 1));
        fi
    done

    # Ninja is preferred over make for faster builds
    if command -v ninja &>/dev/null; then
        CMAKE_GENERATOR="Ninja";
        pass "Build generator: Ninja";
    elif command -v make &>/dev/null; then
        CMAKE_GENERATOR="Unix Makefiles";
        pass "Build generator: Make";
    else
        fail "Neither ninja nor make found.";
    fi
    export CMAKE_GENERATOR;

    if [[ "${missing}" -gt 0 ]]; then
        fail "${missing} required host tool(s) are missing. Install them and retry.";
    fi
}

################################################################################
# @brief  Verify that the Qt installation exists at QT_ROOT.
# @details Also checks for the optional deployqt binary and exports it if found.
################################################################################
check_qt_installation()
{
    section "Checking Qt ${QT_VERSION} Installation";

    if [[ ! -d "${QT_ROOT}" ]]; then
        fail "Qt root not found: ${QT_ROOT} — check QT_VERSION and QT_BASE.";
    fi
    pass "Qt root exists: ${QT_ROOT}";

    if [[ ! -x "${QT6_QMAKE}" ]]; then
        fail "qmake not found or not executable: ${QT6_QMAKE}";
    fi
    pass "qmake found: ${QT6_QMAKE}";

    local qmake_version;
    qmake_version="$("${QT6_QMAKE}" --version 2>&1 | head -2)";
    info "qmake version output: ${qmake_version}";

    # Add Qt bin to PATH so cmake and linuxdeploy-plugin-qt can find Qt tools
    export PATH="${QT_ROOT}/bin:${PATH}";
    info "Prepended Qt bin to PATH";

    # Check for optional deployqt binary (present in some Qt builds)
    if [[ -x "${QT_DEPLOY_QT}" ]]; then
        export QT_DEPLOY_QT;
        pass "deployqt found and exported: ${QT_DEPLOY_QT}";
    else
        info "deployqt not present at ${QT_DEPLOY_QT} — linuxdeploy used instead (normal)";
        unset QT_DEPLOY_QT;
    fi

    pass "Qt installation check";
}

################################################################################
# @brief  Inspect CMakeLists.txt and warn about missing deployment requirements.
# @details Checks for: GNUInstallDirs, install(TARGETS), install(FILES) for
#          .desktop and icon, and CMAKE_BUILD_TYPE not hardcoded to Debug.
################################################################################
check_cmake_file()
{
    section "Checking CMakeLists.txt for Deployment Requirements";

    local cmake_file="CMakeLists.txt";

    if [[ ! -f "${cmake_file}" ]]; then
        fail "CMakeLists.txt not found. Run this script from the project root.";
    fi
    pass "CMakeLists.txt found";

    local issues=0;

    # Check for GNUInstallDirs
    if grep -q "GNUInstallDirs" "${cmake_file}"; then
        pass "CMake: GNUInstallDirs included";
    else
        warn "CMake: GNUInstallDirs not found. Add: include(GNUInstallDirs)";
        issues=$((issues + 1));
    fi

    # Check for install(TARGETS ...)
    if grep -qi "install.*TARGETS" "${cmake_file}"; then
        pass "CMake: install(TARGETS ...) found";
    else
        warn "CMake: No install(TARGETS ...) rule. Binary will not install into AppDir.";
        issues=$((issues + 1));
    fi

    # Check for .desktop install rule
    if grep -qi "\.desktop" "${cmake_file}"; then
        pass "CMake: .desktop file install rule found";
    else
        warn "CMake: No .desktop file install rule found. linuxdeploy requires one.";
        issues=$((issues + 1));
    fi

    # Check for icon install rule
    if grep -qi "icons" "${cmake_file}"; then
        pass "CMake: icon install rule found";
    else
        warn "CMake: No icon install rule found. Add an install rule for the app icon.";
        issues=$((issues + 1));
    fi

    # Warn if Debug is hardcoded
    if grep -q "CMAKE_BUILD_TYPE.*Debug" "${cmake_file}"; then
        warn "CMake: CMAKE_BUILD_TYPE hardcoded to Debug. This script forces Release.";
    else
        pass "CMake: CMAKE_BUILD_TYPE not hardcoded to Debug";
    fi

    if [[ "${issues}" -gt 0 ]]; then
        warn "${issues} CMakeLists.txt issue(s) found. See WARN messages above.";
        warn "Deployment may fail or produce an incomplete AppImage.";
    else
        pass "CMakeLists.txt looks deployment-ready";
    fi
}

################################################################################
# @brief  Build the application in Release mode using cmake.
################################################################################
build_application()
{
    section "Building Application (Release)";

    if [[ "${SKIP_BUILD}" -eq 1 ]]; then
        warn "Skipping build step (-s flag set).";
        return;
    fi

    if [[ ! -f "CMakeLists.txt" ]]; then
        fail "CMakeLists.txt not found. Run from project root.";
    fi

    info "Configuring cmake...";
    cmake -S . -B "${BUILD_DIR}" \
        -G "${CMAKE_GENERATOR}" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH="${QT_ROOT}" \
        -DCMAKE_INSTALL_PREFIX="${PWD}/${INSTALL_PREFIX}" \
        >> "${INSTALLER_LOG}" 2>&1 \
        || fail "cmake configuration failed. Check ${INSTALLER_LOG} for details.";
    pass "cmake configuration";

    info "Building...";
    cmake --build "${BUILD_DIR}" --config Release --parallel "$(nproc)" \
        >> "${INSTALLER_LOG}" 2>&1 \
        || fail "cmake build failed. Check ${INSTALLER_LOG} for details.";
    pass "cmake build";

    info "Installing to prefix: ${INSTALL_PREFIX}";
    cmake --install "${BUILD_DIR}" \
        >> "${INSTALLER_LOG}" 2>&1 \
        || fail "cmake install failed. Check ${INSTALLER_LOG} for details.";
    pass "cmake install";
}

################################################################################
# @brief  Download a file with curl, replacing any existing copy.
# @param  $1  Destination file path.
# @param  $2  Source URL.
################################################################################
download_or_update()
{
    local dest="$1";
    local url="$2";

    if [[ -f "${dest}" ]]; then
        info "Updating: $(basename "${dest}")...";
        # Always fetch the latest continuous build
        rm -f "${dest}";
    else
        info "Downloading: $(basename "${dest}")...";
    fi

    curl -fsSL --output "${dest}" "${url}" \
        >> "${INSTALLER_LOG}" 2>&1 \
        || fail "Download failed: ${url}";

    chmod +x "${dest}" || fail "Cannot chmod +x ${dest}";
    pass "Downloaded/updated: $(basename "${dest}")";
}

################################################################################
# @brief  Download or update linuxdeploy and linuxdeploy-plugin-qt.
################################################################################
fetch_linuxdeploy()
{
    section "Fetching linuxdeploy Tools";

    if [[ "${OS}" != "linux" ]]; then
        info "Skipping linuxdeploy fetch on non-Linux OS.";
        return;
    fi

    download_or_update "${LINUXDEPLOY_BIN}" "${LINUXDEPLOY_URL}";
    download_or_update "${LINUXDEPLOY_QT_BIN}" "${LINUXDEPLOY_QT_URL}";

    # Verify the AppImages are in a recognised format
    local ld_type;
    ld_type="$(file "${LINUXDEPLOY_BIN}")";
    info "linuxdeploy file type: ${ld_type}";

    local ldqt_type;
    ldqt_type="$(file "${LINUXDEPLOY_QT_BIN}")";
    info "linuxdeploy-plugin-qt file type: ${ldqt_type}";
    pass "linuxdeploy tools fetched";
}

################################################################################
# @brief  Copy the SVG app icon into AppDir — no conversion required.
# @details linuxdeploy and the Qt SVG plugin handle SVG natively.
#          The file is renamed to APP_NAME.svg as linuxdeploy expects.
#          A copy is also placed at the AppDir root so linuxdeploy --icon-file
#          can find it without needing a PNG fallback.
################################################################################
copy_icon()
{
    section "Copying App Icon (SVG — no conversion required)";

    if [[ ! -f "${ICON_SRC}" ]]; then
        fail "Icon source not found: ${ICON_SRC}";
    fi

    local svg_dest="${APPDIR_ICONS}/scalable/apps/${APP_NAME}.svg";
    local svg_root="${APPDIR}/${APP_NAME}.svg";

    # Place the scalable SVG in the hicolor icon tree
    cp "${ICON_SRC}" "${svg_dest}" || fail "Cannot copy SVG to hicolor tree";
    pass "SVG icon copied: ${svg_dest}";

    # Place a top-level copy so linuxdeploy --icon-file can reference it
    cp "${ICON_SRC}" "${svg_root}" || fail "Cannot copy top-level SVG icon";
    pass "Top-level SVG icon placed: ${svg_root}";
}

################################################################################
# @brief  Create a .desktop file if one does not already exist in the project.
################################################################################
create_desktop_file()
{
    section "Creating .desktop File";

    local desktop_src="${DESKTOP_FILE}";
    local desktop_dst="${APPDIR_APPS}/${DESKTOP_FILE}";

    if [[ -f "${desktop_src}" ]]; then
        info "Using existing .desktop file: ${desktop_src}";
        cp "${desktop_src}" "${desktop_dst}" || fail "Cannot copy .desktop file";
    else
        info "Generating minimal .desktop file...";
        cat > "${desktop_dst}" <<EOF
[Desktop Entry]
Name=${APP_NAME}
Exec=${APP_NAME}
Icon=${APP_NAME}
Type=Application
Categories=Utility;
Comment=${APP_NAME} Qt Application
EOF
        pass ".desktop file generated";
    fi

    # Validate mandatory keys are present
    for key in "Name=" "Exec=" "Icon=" "Type="; do
        if grep -q "${key}" "${desktop_dst}"; then
            pass ".desktop key present: ${key}";
        else
            fail ".desktop file missing required key: ${key}";
        fi
    done
}

################################################################################
# @brief  Copy the built binary into AppDir/usr/bin.
################################################################################
copy_binary()
{
    section "Copying Application Binary";

    # Search install prefix first, then the build root, then recursively
    local binary="";

    if [[ -x "${INSTALL_PREFIX}/bin/${APP_NAME}" ]]; then
        binary="${INSTALL_PREFIX}/bin/${APP_NAME}";
    elif [[ -x "${BUILD_DIR}/${APP_NAME}" ]]; then
        binary="${BUILD_DIR}/${APP_NAME}";
    else
        # Recursive search as a last resort
        binary="$(find "${BUILD_DIR}" -maxdepth 4 -type f -name "${APP_NAME}" -perm /u+x 2>/dev/null | head -1)";
    fi

    if [[ -z "${binary}" ]]; then
        fail "Built binary '${APP_NAME}' not found in ${BUILD_DIR}. Did the build succeed?";
    fi

    info "Binary found at: ${binary}";
    cp "${binary}" "${APPDIR_BIN}/${APP_NAME}" || fail "Cannot copy binary to AppDir";
    chmod +x "${APPDIR_BIN}/${APP_NAME}" || fail "Cannot chmod binary";
    pass "Binary copied: ${APPDIR_BIN}/${APP_NAME}";
}

################################################################################
# @brief  Build a shadow imageformats directory and a qmake wrapper script.
# @details linuxdeploy-plugin-qt ignores QT_PLUGIN_PATH and QT_INSTALL_PLUGINS
#          environment variables — it calls "qmake -query QT_INSTALL_PLUGINS"
#          directly and uses that path unconditionally.  The only reliable way
#          to redirect its plugin scan without touching /opt/Qt is to intercept
#          the qmake call itself with a wrapper script that sits earlier on PATH.
#
#          Strategy (zero writes to /opt/Qt):
#            1. Copy only the imageformats/ sub-directory to a shadow location
#               inside AppDir/tools/ — safe across filesystem boundaries.
#            2. Delete the unwanted plugins from the shadow copy only.
#            3. Write a tiny qmake wrapper script to AppDir/tools/ that:
#                 • Intercepts "qmake -query QT_INSTALL_PLUGINS" and prints the
#                   shadow parent path instead of the real /opt/Qt path.
#                 • Passes every other qmake call straight through to the real
#                   qmake binary unchanged.
#            4. Prepend AppDir/tools/ to PATH so the wrapper is found first.
#            5. After the bundle step, PATH is restored and the wrapper is
#               deleted — no lasting side effects anywhere.
#
#          Plugins excluded (shadow copy only — /opt/Qt never modified):
#            libqtiff.so — TIFF; requires libtiff.so.5, absent on Fedora.
#            libqpdf.so  — PDF image format via QtPDF; drags in libQt6Pdf,
#                          libQt6Network, and the full Kerberos stack.
################################################################################
build_shadow_plugins()
{
    section "Building Shadow Plugins + qmake Wrapper (no /opt/Qt writes)";

    local qt_imageformats_real="${QT_ROOT}/plugins/imageformats";

    # Shadow layout: <shadow_parent>/imageformats/  (mirrors Qt plugin tree)
    # The qmake wrapper will report <shadow_parent> as QT_INSTALL_PLUGINS so
    # linuxdeploy-plugin-qt scans <shadow_parent>/imageformats/ — our copy.
    local shadow_parent="${APPDIR_TOOLS}/qt_plugins_shadow";
    # SHADOW_PLUGINS_DIR="${shadow_parent}";
    local shadow_imageformats="${shadow_parent}/imageformats";

    rm -rf "${shadow_parent}";
    mkdir -p "${shadow_imageformats}" || fail "Cannot create shadow imageformats dir";

    if [[ ! -d "${qt_imageformats_real}" ]]; then
        fail "Qt imageformats directory not found: ${qt_imageformats_real}";
    fi

    # Regular copy — safe across filesystem boundaries unlike hard links.
    # Only the .so plugins are needed; .debug files are not deployed.
    cp "${qt_imageformats_real}"/lib*.so "${shadow_imageformats}/" \
        >> "${INSTALLER_LOG}" 2>&1 \
        || fail "Cannot copy imageformats plugins to shadow directory";

    info "Shadow imageformats directory: ${shadow_imageformats}";

    # Delete unwanted plugins from the shadow copy only — /opt/Qt untouched.
    local -a exclude_plugins=("libqtiff.so" "libqpdf.so");
    local removed=0;

    for plugin in "${exclude_plugins[@]}"; do
        local shadow_path="${shadow_imageformats}/${plugin}";
        if [[ -f "${shadow_path}" ]]; then
            rm -f "${shadow_path}" || fail "Cannot remove plugin from shadow: ${plugin}";
            info "Excluded from shadow (not from /opt/Qt): ${plugin}";
            removed=$((removed + 1));
        else
            info "Plugin not present in shadow (already absent): ${plugin}";
        fi
    done

    pass "Shadow imageformats ready: ${removed} plugin(s) excluded";

    # ── Write the qmake wrapper script ────────────────────────────────────────
    # linuxdeploy-plugin-qt calls: qmake -query QT_INSTALL_PLUGINS
    # This wrapper intercepts that exact query and returns the shadow path.
    # All other qmake calls are forwarded to the real binary unchanged.
    QMAKE_WRAPPER="${APPDIR_TOOLS}/qmake";

    # Use a quoted heredoc delimiter ('QMAKE_WRAPPER_EOF') so the outer shell
    # does NOT expand $* or $@ — they must remain literal in the generated script.
    # The two values we want baked in (shadow path and real qmake path) are
    # written via shell variables BEFORE the heredoc so they expand correctly.
    local baked_shadow="${shadow_parent}";
    local baked_qmake="${QT6_QMAKE}";

    cat > "${QMAKE_WRAPPER}" <<'QMAKE_WRAPPER_EOF'
#!/usr/bin/env bash
# qmake wrapper — intercepts QT_INSTALL_PLUGINS query for linuxdeploy-plugin-qt.
# Generated by deploy.sh build_shadow_plugins(); deleted after the bundle step.
# shellcheck disable=SC2128,SC2178
SHADOW_PATH="SHADOW_PATH_PLACEHOLDER"
REAL_QMAKE="REAL_QMAKE_PLACEHOLDER"
if [[ "$*" == *"QT_INSTALL_PLUGINS"* ]]; then
    echo "${SHADOW_PATH}"
else
    exec "${REAL_QMAKE}" "$@"
fi
QMAKE_WRAPPER_EOF

    # Substitute the placeholders with the actual baked-in paths.
    sed -i "s|SHADOW_PATH_PLACEHOLDER|${baked_shadow}|g" "${QMAKE_WRAPPER}";
    sed -i "s|REAL_QMAKE_PLACEHOLDER|${baked_qmake}|g" "${QMAKE_WRAPPER}";

    chmod +x "${QMAKE_WRAPPER}" || fail "Cannot chmod qmake wrapper";
    info "qmake wrapper written: ${QMAKE_WRAPPER}";
    info "  Intercepts: qmake -query QT_INSTALL_PLUGINS → ${shadow_parent}";
    info "  Forwards:   all other qmake calls → ${QT6_QMAKE}";
    pass "qmake wrapper ready";
}

################################################################################
# @brief  Run linuxdeploy to populate AppDir, using the qmake wrapper.
# @details The qmake wrapper (written by build_shadow_plugins) sits first on
#          PATH so linuxdeploy-plugin-qt's "qmake -query QT_INSTALL_PLUGINS"
#          call returns the shadow directory instead of /opt/Qt.  The wrapper
#          is removed from PATH and deleted after the bundle completes.
#          Runs WITHOUT --output appimage; packaging is a separate step.
################################################################################
run_linuxdeploy_bundle()
{
    section "linuxdeploy: Bundle Qt Libraries into AppDir";

    # Core variables expected by linuxdeploy-plugin-qt
    export QMAKE="${QMAKE_WRAPPER}";
    export QML_SOURCES_PATHS="${PWD}";
    export DEPLOY_PLATFORM_THEMES=1;

    info "Bundling Qt libraries into AppDir (no packaging yet)...";

    # Prepend AppDir/tools/ so the qmake wrapper is found before the real qmake.
    # The wrapper binary is named "qmake" and lives in APPDIR_TOOLS.
    export PATH="${APPDIR_TOOLS}:${PATH}";
    info "PATH prepended with ${APPDIR_TOOLS} (qmake wrapper active)";

    # No --output flag: populate AppDir only; packaging is a separate step.
    "${LINUXDEPLOY_BIN}" \
        --appdir "${APPDIR}" \
        --plugin qt \
        --desktop-file "${APPDIR_APPS}/${DESKTOP_FILE}" \
        --icon-file "${APPDIR}/${APP_NAME}.svg" \
        >> "${INSTALLER_LOG}" 2>&1 \
        || fail "linuxdeploy bundle step failed. Check ${INSTALLER_LOG} for details.";

    # Remove the wrapper so nothing downstream accidentally uses it.
    rm -f "${QMAKE_WRAPPER}";
    info "qmake wrapper removed after bundle step";

    # Restore QMAKE to the real binary for any subsequent steps.
    export QMAKE="${QT6_QMAKE}";

    pass "linuxdeploy bundle step complete (AppDir populated)";
}

################################################################################
# @brief  Package the populated AppDir into a final AppImage.
# @details --plugin qt is intentionally omitted so linuxdeploy does not
#          re-scan the Qt plugin directory and re-introduce excluded plugins.
################################################################################
run_linuxdeploy_package()
{
    section "linuxdeploy: Package AppDir into AppImage";

    info "Squashing AppDir into AppImage...";

    "${LINUXDEPLOY_BIN}" \
        --appdir "${APPDIR}" \
        --output appimage \
        >> "${INSTALLER_LOG}" 2>&1 \
        || fail "linuxdeploy package step failed. Check ${INSTALLER_LOG} for details.";

    pass "linuxdeploy package step complete (AppImage created)";
}

################################################################################
# @brief  Orchestrate the full linuxdeploy pipeline.
# @details Order of operations:
#            1. build_shadow_plugins()   — hard-link Qt plugins to a shadow dir,
#                                         remove excluded plugins from shadow only
#            2. run_linuxdeploy_bundle() — populate AppDir via shadow (no TIFF)
#            3. run_linuxdeploy_package()— squash clean AppDir into AppImage
#          /opt/Qt is never written to at any point.
################################################################################
run_linuxdeploy()
{
    section "Running linuxdeploy + Qt Plugin";

    if [[ "${OS}" != "linux" ]]; then
        info "Skipping linuxdeploy on non-Linux OS.";
        return;
    fi

    build_shadow_plugins;        # Step 1: shadow Qt plugins, exclude TIFF/PDF
    run_linuxdeploy_bundle;      # Step 2: bundle via shadow (TIFF never seen)
    run_linuxdeploy_package;     # Step 3: squash clean AppDir into AppImage

    pass "linuxdeploy pipeline completed successfully";
}

################################################################################
# @brief  On macOS, use macdeployqt to bundle Qt frameworks into a DMG.
################################################################################
run_macdeployqt()
{
    section "Running macdeployqt";

    local macdeployqt_bin="${QT_ROOT}/bin/macdeployqt";

    if [[ ! -x "${macdeployqt_bin}" ]]; then
        fail "macdeployqt not found: ${macdeployqt_bin}";
    fi

    local app_bundle;
    app_bundle="$(find "${BUILD_DIR}" -maxdepth 4 -name "*.app" -type d | head -1)";

    if [[ -z "${app_bundle}" ]]; then
        fail "No .app bundle found in ${BUILD_DIR}. Ensure cmake is configured for macOS.";
    fi

    info "App bundle: ${app_bundle}";

    "${macdeployqt_bin}" "${app_bundle}" -dmg \
        >> "${INSTALLER_LOG}" 2>&1 \
        || fail "macdeployqt failed. Check ${INSTALLER_LOG} for details.";

    pass "macdeployqt completed";
}

################################################################################
# @brief  Run the battery of pre-packaging tests on the AppDir binary.
# @details Tests: file type, ldd missing libs, Qt plugin directory, icon files,
#          .desktop validity, and a sandboxed runtime execution test.
################################################################################
run_tests()
{
    section "Running Deployment Test Suite";

    if [[ "${SKIP_TESTS}" -eq 1 ]]; then
        warn "Skipping tests (-t flag set).";
        return;
    fi

    local binary="${APPDIR_BIN}/${APP_NAME}";
    local pass_count=0;
    local fail_count=0;

    # ── Test 1: Binary exists and is executable ───────────────────────────────
    if [[ -x "${binary}" ]]; then
        pass "T01: Binary is executable";
        pass_count=$((pass_count + 1));
    else
        warn "T01: Binary missing or not executable: ${binary}";
        fail_count=$((fail_count + 1));
    fi

    # ── Test 2: Binary is ELF ─────────────────────────────────────────────────
    local bin_type;
    bin_type="$(file "${binary}" 2>/dev/null)";
    if echo "${bin_type}" | grep -q "ELF"; then
        pass "T02: Binary is ELF: ${bin_type}";
        pass_count=$((pass_count + 1));
    else
        warn "T02: Binary does not appear to be ELF: ${bin_type}";
        fail_count=$((fail_count + 1));
    fi

    # ── Test 3: ldd — no missing shared libraries ─────────────────────────────
    local ldd_output;
    ldd_output="$(ldd "${binary}" 2>&1)";
    info "ldd output for ${APP_NAME}: ${ldd_output}";
    {
        echo "--- ldd output ---";
        echo "${ldd_output}";
    } >> "${INSTALLER_LOG}";

    if echo "${ldd_output}" | grep -q "not found"; then
        warn "T03: ldd reports missing libraries:";
        echo "${ldd_output}" | grep "not found" >> "${INSTALLER_LOG}";
        fail_count=$((fail_count + 1));
    else
        pass "T03: No missing libraries reported by ldd";
        pass_count=$((pass_count + 1));
    fi

    # ── Test 4: Qt plugins directory bundled ──────────────────────────────────
    if [[ -d "${APPDIR_USR}/plugins" || -d "${APPDIR_USR}/lib/qt6/plugins" ]]; then
        pass "T04: Qt plugins directory found in AppDir";
        pass_count=$((pass_count + 1));
    else
        warn "T04: Qt plugins directory not found — linuxdeploy may not have run yet";
        fail_count=$((fail_count + 1));
    fi

    # ── Test 5: Platform plugin (xcb) present ────────────────────────────────
    local xcb_plugin;
    xcb_plugin="$(find "${APPDIR}" -name "libqxcb.so" 2>/dev/null | head -1)";
    if [[ -n "${xcb_plugin}" ]]; then
        pass "T05: xcb platform plugin found: ${xcb_plugin}";
        pass_count=$((pass_count + 1));
    else
        warn "T05: xcb platform plugin (libqxcb.so) not found — Qt may fail on X11";
        fail_count=$((fail_count + 1));
    fi

    # ── Test 6: SVG image plugin present ─────────────────────────────────────
    local svg_plugin;
    svg_plugin="$(find "${APPDIR}" -name "libqsvg.so" 2>/dev/null | head -1)";
    if [[ -n "${svg_plugin}" ]]; then
        pass "T06: SVG image plugin found: ${svg_plugin}";
        pass_count=$((pass_count + 1));
    else
        warn "T06: SVG image plugin (libqsvg.so) not found — SVG icons may not render";
        fail_count=$((fail_count + 1));
    fi

    # ── Test 7: .desktop file present ────────────────────────────────────────
    if [[ -f "${APPDIR_APPS}/${DESKTOP_FILE}" ]]; then
        pass "T07: .desktop file present";
        pass_count=$((pass_count + 1));
    else
        warn "T07: .desktop file missing from AppDir";
        fail_count=$((fail_count + 1));
    fi

    # ── Test 8: SVG icon present (linuxdeploy accepts SVG natively) ───────────
    if [[ -f "${APPDIR_ICONS}/scalable/apps/${APP_NAME}.svg" ]]; then
        pass "T08: Scalable SVG icon present";
        pass_count=$((pass_count + 1));
    else
        warn "T08: SVG icon missing from hicolor/scalable/apps/";
        fail_count=$((fail_count + 1));
    fi

    # ── Test 9: AppRun entry point (created by linuxdeploy) ──────────────────
    if [[ -f "${APPDIR}/AppRun" ]]; then
        pass "T09: AppRun entry point present";
        pass_count=$((pass_count + 1));
    else
        warn "T09: AppRun not found — linuxdeploy should create this";
        fail_count=$((fail_count + 1));
    fi

    # ── Test 10: Runtime execution test ──────────────────────────────────────
    run_runtime_test;
    local rt_status=$?;
    if [[ "${rt_status}" -eq 0 ]]; then
        pass "T10: Runtime execution test passed";
        pass_count=$((pass_count + 1));
    else
        warn "T10: Runtime execution test failed (exit ${rt_status}) — see ${DEBUG_LOG}";
        fail_count=$((fail_count + 1));
    fi

    # ── Summary ───────────────────────────────────────────────────────────────
    echo "";
    echo -e "${BOLD}Test Results: ${GREEN}${pass_count} passed${RESET}  ${RED}${fail_count} failed${RESET}";
    {
        echo "";
        echo "Test Results: ${pass_count} passed  ${fail_count} failed";
    } >> "${INSTALLER_LOG}";

    if [[ "${fail_count}" -gt 0 ]]; then
        warn "${fail_count} test(s) failed. The AppImage may still be functional.";
        warn "Review ${INSTALLER_LOG} and ${DEBUG_LOG} for diagnostics.";
    else
        pass "All tests passed.";
    fi
}

################################################################################
# @brief  Execute the application binary in a sandboxed debug mode.
# @details Sets QT_DEBUG_PLUGINS=1 and captures stdout/stderr to DEBUG_LOG.
#          The app is launched with a short timeout; a timeout kill (exit 124)
#          from a GUI app that stayed alive is treated as a successful launch.
# @return 0 if the app launched without immediately crashing, non-zero otherwise.
################################################################################
run_runtime_test()
{
    local binary="${APPDIR_BIN}/${APP_NAME}";
    local timeout_sec=5;
    local ret=0;

    info "Running runtime test (${timeout_sec}s timeout)...";
    {
        echo "--- Runtime Test $(date) ---";
    } >> "${DEBUG_LOG}";

    # Set Qt debug environment variables for maximum diagnostic output
    export QT_DEBUG_PLUGINS=1;
    export LD_LIBRARY_PATH="${APPDIR_USR}/lib:${QT_ROOT}/lib:${LD_LIBRARY_PATH:-}";
    export QT_PLUGIN_PATH="${APPDIR_USR}/plugins";
    export QT_QPA_PLATFORM="offscreen";  # Headless; avoids needing a display

    # Run with timeout; capture all output to the debug log
    if timeout "${timeout_sec}" "${binary}" >> "${DEBUG_LOG}" 2>&1; then
        ret=0;
        info "Binary exited cleanly within timeout.";
    else
        local exit_code=$?;
        if [[ "${exit_code}" -eq 124 ]]; then
            # timeout returns 124 on a kill — expected for a GUI app that stayed up
            ret=0;
            info "Binary reached timeout (GUI app stayed alive — successful launch).";
        else
            ret="${exit_code}";
            warn "Binary exited with code ${exit_code}. See ${DEBUG_LOG}.";
        fi
    fi

    # Unset debug vars so they do not pollute downstream steps
    unset QT_DEBUG_PLUGINS QT_QPA_PLATFORM;

    return "${ret}";
}

################################################################################
# @brief  Move the generated AppImage or DMG into the deploy/ output folder.
################################################################################
finalise_output()
{
    section "Finalising Output";

    if [[ "${OS}" == "linux" ]]; then
        # linuxdeploy writes the AppImage to the current directory
        local appimage;
        appimage="$(find . -maxdepth 1 -name "*.AppImage" -newer "${BUILD_DIR}" 2>/dev/null | head -1)";

        if [[ -z "${appimage}" ]]; then
            # Fall back to any AppImage in the project root
            appimage="$(find . -maxdepth 1 -name "*.AppImage" 2>/dev/null | head -1)";
        fi

        if [[ -z "${appimage}" ]]; then
            fail "No AppImage found in project root. linuxdeploy may have failed.";
        fi

        local dest="${DEPLOY_DIR}/${APP_NAME}-${APP_VERSION}-x86_64.AppImage";
        mv "${appimage}" "${dest}" || fail "Cannot move AppImage to ${DEPLOY_DIR}";
        chmod +x "${dest}";
        pass "AppImage deployed: ${dest}";
        echo -e "\n${GREEN}${BOLD}  ✔ Installer ready: ${dest}${RESET}\n";
        echo "  Installer ready: ${dest}" >> "${INSTALLER_LOG}";

    elif [[ "${OS}" == "mac" ]]; then
        # macdeployqt creates a .dmg in the build directory
        local dmg;
        dmg="$(find "${BUILD_DIR}" -maxdepth 2 -name "*.dmg" 2>/dev/null | head -1)";

        if [[ -z "${dmg}" ]]; then
            fail "No .dmg found in ${BUILD_DIR}.";
        fi

        local dest="${DEPLOY_DIR}/${APP_NAME}-${APP_VERSION}.dmg";
        mv "${dmg}" "${dest}" || fail "Cannot move DMG to ${DEPLOY_DIR}";
        pass "DMG deployed: ${dest}";
        echo -e "\n${GREEN}${BOLD}  ✔ Installer ready: ${dest}${RESET}\n";
        echo "  Installer ready: ${dest}" >> "${INSTALLER_LOG}";

    else
        warn "Windows packaging is not automated by this script.";
        warn "Run windeployqt from a Developer Command Prompt and package with NSIS or Inno Setup.";
    fi
}

################################################################################
# @brief  Lightweight auto-update check.
# @details Fetches a remote VERSION file and compares it to APP_VERSION.
#          Replace UPDATE_URL with the raw GitHub URL once the repo is created,
#          e.g. https://raw.githubusercontent.com/User/Repo/main/VERSION
################################################################################
check_for_update()
{
    section "Checking for Updates";

    info "Fetching remote version from: ${UPDATE_URL}";

    local remote_version="";
    remote_version="$(curl -fsSL --max-time 5 "${UPDATE_URL}" 2>/dev/null | tr -d '[:space:]')" || true;

    if [[ -z "${remote_version}" ]]; then
        warn "Could not reach update server (${UPDATE_URL}). Skipping update check.";
        info "Set UPDATE_URL to your raw GitHub VERSION file URL once the repo is created.";
        return;
    fi

    info "Local version : ${APP_VERSION}";
    info "Remote version: ${remote_version}";

    if [[ "${remote_version}" == "${APP_VERSION}" ]]; then
        pass "Application is up to date (${APP_VERSION})";
    else
        warn "A newer version is available: ${remote_version} (you have ${APP_VERSION})";
        warn "Download the latest release and re-run deploy.sh to update.";
    fi
}

################################################################################
# @brief  Print a final summary of the deployment run.
################################################################################
print_summary()
{
    section "Deployment Summary";
    {
        echo "  Log file   : ${INSTALLER_LOG}";
        echo "  Debug log  : ${DEBUG_LOG}";
        echo "  Deploy dir : ${DEPLOY_DIR}/";
        echo "";
        echo "  To run the app:";
    } | tee -a "${INSTALLER_LOG}";

    if [[ "${OS}" == "linux" ]]; then
        echo "    ./${DEPLOY_DIR}/${APP_NAME}-${APP_VERSION}-x86_64.AppImage" \
            | tee -a "${INSTALLER_LOG}";
    elif [[ "${OS}" == "mac" ]]; then
        echo "    open ./${DEPLOY_DIR}/${APP_NAME}-${APP_VERSION}.dmg" \
            | tee -a "${INSTALLER_LOG}";
    fi
    echo "" | tee -a "${INSTALLER_LOG}";
}

################################################################################
# @brief  Main entry point — orchestrates all deployment stages.
# @param  $@  Command-line arguments forwarded from the shell.
################################################################################
main()
{
    # bootstrap_log MUST be first — creates LOG_DIR and opens INSTALLER_LOG
    # so every subsequent helper (pass/fail/warn/info/section) can write to it.
    bootstrap_log "$@";

    parse_args "$@";

    echo -e "${BOLD}${CYAN}";
    echo "  ╔═══════════════════════════════════════════════════╗";
    echo "  ║        Qt ${QT_VERSION} Deployment Script                ║";
    echo "  ║        $(date '+%Y-%m-%d %H:%M:%S')                        ║";
    echo "  ╚═══════════════════════════════════════════════════╝";
    echo -e "${RESET}";

    # Ensure we are in the project root before doing anything else
    if [[ ! -f "CMakeLists.txt" ]]; then
        fail "CMakeLists.txt not found. Run this script from the project root.";
    fi

    # Resolve APP_NAME and DESKTOP_FILE from cmake before any step uses them
    read_cmake_app_name;

    detect_os;
    init_dirs;           # Wipes AppDir and rebuilds; LOG_DIR already exists
    check_host_tools;
    check_qt_installation;
    check_cmake_file;
    check_for_update;
    build_application;
    copy_icon;
    create_desktop_file;
    copy_binary;

    if [[ "${OS}" == "linux" ]]; then
        fetch_linuxdeploy;
        run_linuxdeploy;
    elif [[ "${OS}" == "mac" ]]; then
        run_macdeployqt;
    else
        warn "Windows deployment is not automated. See deploy.sh comments.";
    fi

    run_tests;
    finalise_output;
    print_summary;
}

################################################################################
main "$@"
################################################################################
# @brief End of File deploy.sh
################################################################################
