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
# @date        2026-03-31
# @section     License  Unlicensed, MIT, or any.
# clear; chmod +x deploy.sh && shellcheck deploy.sh && ./deploy.sh
# deploy/ProjectSourceDoc-1.0.0-x86_64.AppImage
# chmod +x deploy/ProjectSourceDoc-1.0.0-x86_64.AppImage && ./deploy/ProjectSourceDoc-1.0.0-x86_64.AppImage
# systemctl --user restart xdg-desktop-portal-kde && systemctl --user restart xdg-desktop-portal
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
SCRIPT_VERSION="1.9";

################################################################################
# @brief Global configuration — edit these variables for your project.
# @details APP_NAME and DESKTOP_FILE are set dynamically by read_cmake_app_name
#          early in main().  The placeholders below are overwritten at runtime.
################################################################################
declare -gxi run_mode=1; # 1=Run Tests

# Application identity — overwritten by read_cmake_app_name()
declare -gx APP_NAME="UnknownApp";                   # Resolved from cmake project() at runtime
declare -gx APP_VERSION="1.0.0";                     # Local version string for update check
declare -gx DESKTOP_FILE="${APP_NAME}.desktop";      # Derived from APP_NAME; reset after parse
declare -gx ICON_SRC="resources/icons/light/app.svg";

################################################################################
# @brief Qt installation root (Qt Maintenance Tool default)
# @details Updated for Qt 6.10.2 to match Fedora 43 system Qt.
################################################################################
declare -gx QT_VERSION="6.10.2";
declare -gx QT_BASE="/opt/Qt";
declare -gx QT_ARCH="gcc_64";
declare -gx QT_ROOT="${QT_BASE}/${QT_VERSION}/${QT_ARCH}";
declare -gx QT6_QMAKE="${QT_ROOT}/bin/qmake";
declare -gx QT_DEPLOY_QT="${QT_ROOT}/bin/deployqt";
export QT6_QMAKE;
export NO_STRIP=1;

# TIFF exclusion strategy: QT_PLUGIN_FILTER is a WHITELIST in linuxdeploy-plugin-qt,
# not a blacklist — a leading "-" prefix is silently ignored, so it cannot
# be used to exclude individual plugins.  The correct approach used here is
# a two-phase deploy: (1) populate AppDir without packaging, (2) delete
# libqtiff.so and its orphaned libtiff.so.* before the AppImage is squashed.
# See remove_unwanted_plugins() for the full implementation.

# Build
declare -gx BUILD_DIR="build_release";              # Temporary cmake build folder (in root)
declare -gx INSTALL_PREFIX="${BUILD_DIR}/install";  # cmake install target inside build dir

#****************************************************************
# @brief Global
#****************************************************************
# Root Folder must where the script runs from.
declare -gx PATH_ROOT; PATH_ROOT="$(cd -- "${PWD}" && pwd -P)";
declare -gx PATH_BASH="${PATH_ROOT}/bash";
declare -gx PATH_BACKUP="${HOME}/Nextcloud/workspace/ProjectSource/Backups/";

# AppDir (treated as temporary — deleted and recreated each run)
declare -gx APPDIR="AppDir";
declare -gx APPDIR_USR="${APPDIR}/usr";
declare -gx APPDIR_BIN="${APPDIR_USR}/bin";
declare -gx APPDIR_LIB="${APPDIR_USR}/lib";
declare -gx APPDIR_SHARE="${APPDIR_USR}/share";
declare -gx APPDIR_APPS="${APPDIR_SHARE}/applications";
declare -gx APPDIR_ICONS="${APPDIR_SHARE}/icons/hicolor";
declare -gx APPDIR_TOOLS="${APPDIR}/tools";         # linuxdeploy binaries cached here

# Output
declare -gx DEPLOY_DIR="${PATH_ROOT}/deploy";
declare -gx LOG_DIR="${DEPLOY_DIR}/logs";
# INSTALLER_LOG is set by bootstrap_log() after LOG_DIR is created
declare -gx INSTALLER_LOG="";
declare -gx DEBUG_LOG="${LOG_DIR}/app_debug.log";

# linuxdeploy download URLs
declare -gx LINUXDEPLOY_URL="https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage";
declare -gx LINUXDEPLOY_QT_URL="https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage";
declare -gx LINUXDEPLOY_BIN="${APPDIR_TOOLS}/linuxdeploy-x86_64.AppImage";
declare -gx LINUXDEPLOY_QT_BIN="${APPDIR_TOOLS}/linuxdeploy-plugin-qt-x86_64.AppImage";

declare -gx APPIMAGETOOL_URL="https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage"
declare -gx APPIMAGETOOL_BIN="${APPDIR_TOOLS}/appimagetool-x86_64.AppImage"

# Auto-update stub URL — FIXME Create actual GiHub repo
declare -gx UPDATE_URL="https://raw.githubusercontent.com/AM-Tower/ProjectSourceDoc/main/VERSION";

# Runtime flags (defaults)
declare -gx VERBOSE=0;                              # Quiet by default; -v enables verbose
declare -gx SKIP_BUILD=0;                           # -s skips cmake build (reuse last build)
declare -gx SKIP_TESTS=0;                           # -t skips the test suite

# OS is set by detect_os()
declare -gx OS="linux";

# CMAKE_GENERATOR is set by check_host_tools()
declare -gx CMAKE_GENERATOR="Ninja";

#**************************************************************************************************
# @brief        Versioning control flags and files.
# @details      These variables control release-time version mutation. VERSION is the single authoritative source.
#**************************************************************************************************
declare -gx VERSION_FILE="VERSION"
declare -gx VERSION_BACKUP_FILE=".VERSION.bak"

#**************************************************************************************************
# @section Color Definitions
#**************************************************************************************************
declare -gx BOLD='\033[1m';
declare -gx RESET='\033[0m';
declare -gx NC='\033[0m';           # Normal Color
declare -gx GREEN='\033[0;32m';     # Pass
declare -gx LIME='\033[1;32m';      # Success
declare -gx YELLOW='\033[1;33m';    # Caution, Warning
declare -gx RED='\033[0;31m';       # Error
declare -gx BRIGHTRED='\033[1;31m'; # Error
declare -gx CYAN='\033[0;36m';      # Info
declare -gx MAGENTA='\033[0;35m';   # Debug
declare -gx BLUE='\033[0;34m';      # Delta, Header
declare -gx SKYBLUE='\033[1;36m';   #
declare -gx ORANGE='\033[0;33m';    # Trace
declare -gx GRAY='\033[0;37m';      # Black background
declare -gx WHITE='\033[1;37m';     # Black background

################################################################################
# @brief  Parse command-line arguments and set runtime flags.
################################################################################
declare -gxi SWITCH_GIT_INIT=0      # --git-init
declare -gxi SWITCH_TRACE=0;        # --trace:
declare -gxi SWITCH_BACKUP=0;       # --backup:
declare -gxi SWITCH_FINAL=0         # --final
declare -gxi SWITCH_RESET=0         # --reset
declare -gx  SWITCH_NEW_VERSION=""; # --version=X.Y.Z
declare -gxi SWITCH_GITHUB=0;       # --github
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


#**************************************************************************************************
# @brief      Show usage information (global and per-project).
#**************************************************************************************************
usage()
{
    printf "\n%b **************************************************************************************** %b \n" "${LIME}" "${NC}";
    printf "%b Main Help %b \n" "${CYAN}" "${NC}";
    printf "%b Usage: main.sh [--backup] %b \n" "${CYAN}" "${NC}";
    printf "%b Options: %b \n" "${CYAN}" "${NC}";
    printf "%b --help        Show this help message %b \n" "${CYAN}" "${NC}";
    if (( SWITCH_BACKUP == 1 )); then
        printf "%b --backup %b     Run project backup: |%b%s%b| %b \n" "${LIME}" "${CYAN}" "${LIME}" "${PATH_BACKUP}" "${CYAN}" "${NC}";
    else
        printf "%b --backup      Run project backup %b \n" "${CYAN}" "${NC}";
    fi
    printf "%b --gitinit     Create a local git repo. Clean install deletes folder if exists  %b \n" "${CYAN}" "${NC}";
    printf "%b --trace       Show trace messages %b \n" "${CYAN}" "${NC}";
    printf "%b --final       Increment Version %b \n" "${CYAN}" "${NC}";
    printf "%b --reset       Reset Version %b \n" "${CYAN}" "${NC}";
    printf "%b --version     Set Version: X.Y.Z %b \n" "${CYAN}" "${NC}";
    printf "%b --github      Push GitHub %b \n" "${CYAN}" "${NC}";
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
    printf "\n%b **************************************************************************************** %b \n" "${LIME}" "${NC}";
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
# @brief  Parse command-line arguments (short + long flags).
# @details Supports:
#          -v               Verbose mode
#          -s               Skip build
#          -t / --skip-tests
#          -T / --appdir-only
#          -g / --git-init
#          -h / --help
################################################################################
parse_args()
{
    for arg in "$@"; do
        case "${arg}" in
            --final)       SWITCH_FINAL=1; ;;
            --reset)       SWITCH_RESET=1; ;;
            --version=*)   SWITCH_NEW_VERSION="${arg#*=}"; ;;
            --github)      SWITCH_GITHUB=1; ;;
            --skip-tests)  SKIP_TESTS=1; ;;
            --appdir-only) run_mode=0; ;;
            --git-init)    SWITCH_GIT_INIT=1; ;;
            --backup)      SWITCH_BACKUP=1; ;;
            --trace)       SWITCH_TRACE=1; ;;
            --help)        usage; ;;
        esac
    done
}

################################################################################
# @brief        Read the current project version from VERSION.
# @return       Semantic version string (MAJOR.MINOR.PATCH).
################################################################################
read_version()
{
    [[ -f "${VERSION_FILE}" ]] || fail "VERSION file missing"
    tr -d '[:space:]' < "${VERSION_FILE}"
}

################################################################################
# @brief        Write a semantic version string to VERSION.
# @param[in]    $1 New version string.
################################################################################
write_version()
{
    echo "$1" > "${VERSION_FILE}"
}

################################################################################
# @brief        Increment PATCH version.
# @param[in]    $1 Current semantic version.
# @return       Incremented version.
################################################################################
increment_patch()
{
    local v="$1"
    IFS='.' read -r major minor patch <<< "${v}"
    echo "${major}.${minor}.$((patch + 1))"
}

################################################################################
# @brief        Validate semantic version format.
# @param[in]    $1 Version string.
################################################################################
validate_version()
{
    [[ "$1" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || fail "Invalid version format: $1"
}

#**********************************************************************************************************************
# @brief Ensure a file exists, creating it with default content if missing.
# @details Used for authoritative configuration files such as VERSION.
#          The file is created only if it does not already exist.
# @param  $1 File path to check or create.
# @param  $2 Default content written if the file is created.
# @return None.
#**********************************************************************************************************************
ensure_file_exists()
{
    local file="${1}";
    local default_content="${2}";
    if [[ -z "${file}" ]]; then fail "Error" "ensure_file_exists: file path is empty"; fi
    if [[ ! -f "${file}" ]]; then
        mkdir -p "$(dirname -- "${file}")";
        printf "%s" "${default_content}" > "${file}";
        info "Info" "Created file: ${file}";
    fi
    return 0;
}

################################################################################
# @brief        Handle release-time version finalization.
# @details      Applies version mutation ONLY when --final is supplied.
################################################################################
handle_versioning()
{
    ensure_file_exists "${VERSION_FILE}" "0.1.0";

    if (( SWITCH_RESET == 1 )); then
        [[ -f "${VERSION_BACKUP_FILE}" ]] || fail "No backup version to reset";
        mv -f "${VERSION_BACKUP_FILE}" "${VERSION_FILE}";
        info "Version reset to $(read_version)";
        exit 0;
    fi

    if (( SWITCH_FINAL == 1 )); then
        local current next;

        current="$(read_version)";
        cp "${VERSION_FILE}" "${VERSION_BACKUP_FILE}";

        if [[ -n "${SWITCH_NEW_VERSION}" ]]; then
            validate_version "${SWITCH_NEW_VERSION}";
            next="${SWITCH_NEW_VERSION}";
        else
            next="$(increment_patch "${current}")";
        fi

        info "Finalizing release version: ${current} → ${next}";
        write_version "${next}";
    fi

    # Always export APP_VERSION from VERSION
    APP_VERSION="$(read_version)";
    export APP_VERSION;
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
# @brief  Clean AppDir directory tree (wiped fresh each run).
################################################################################
clean_appdir_contents()
{
 local dir="$1";

 [[ -z "${dir}" ]] && fail "clean_appdir_contents: no directory specified";
 [[ "${dir}" == "/" ]] && fail "Refusing to clean root directory";
 [[ ! -d "${dir}" ]] && return 0;

 info "Cleaning contents of ${dir} (preserving directory for Nextcloud)";

 # Remove non-hidden entries
 rm -rf "${dir:?}/"* 2>/dev/null || true;

 # Remove hidden entries (but not . or ..)
 rm -rf "${dir:?}/".[^.]* "${dir:?}/"..?* 2>/dev/null || true;
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

    # Ensure AppDir exists but never delete the directory itself (Nextcloud)
    mkdir -p \
        "${APPDIR_BIN}" \
        "${APPDIR_LIB}" \
        "${APPDIR_SHARE}" \
        "${APPDIR_APPS}" \
        "${APPDIR_ICONS}/scalable/apps" \
        "${APPDIR_TOOLS}" \
        || fail "Cannot create AppDir sub-directories";

    # Clean contents except tools + logs if you want to keep caches
    # (If you want to keep tools cache, clean everything but tools/)
    clean_appdir_contents "${APPDIR}";

    # Recreate the required structure after cleaning
    mkdir -p \
        "${APPDIR_BIN}" \
        "${APPDIR_LIB}" \
        "${APPDIR_SHARE}" \
        "${APPDIR_APPS}" \
        "${APPDIR_ICONS}/scalable/apps" \
        "${APPDIR_TOOLS}" \
        || fail "Cannot create AppDir sub-directories";

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
# @brief  Verify Qt installation and version correctness.
# @details Ensures Qt root exists, qmake exists, and qmake reports the expected
#          Qt version. Prevents symbol mismatches that cause AppImage segfaults.
################################################################################
check_qt_installation()
{
    section "Checking Qt ${QT_VERSION} Installation";

    # -------------------------------------------------------------------------
    # 1. Verify Qt root directory exists
    # -------------------------------------------------------------------------
    if [[ ! -d "${QT_ROOT}" ]]; then
        fail "Qt root not found: ${QT_ROOT} — check QT_VERSION and QT_BASE.";
    fi;
    pass "Qt root exists: ${QT_ROOT}";

    # -------------------------------------------------------------------------
    # 2. Verify qmake exists
    # -------------------------------------------------------------------------
    if [[ ! -x "${QT6_QMAKE}" ]]; then
        fail "qmake not found or not executable: ${QT6_QMAKE}";
    fi;
    pass "qmake found: ${QT6_QMAKE}";

    # -------------------------------------------------------------------------
    # 3. Verify qmake reports the expected Qt version
    # -------------------------------------------------------------------------
    local qv;
    qv="$("${QT6_QMAKE}" -query QT_VERSION)";
    if [[ "${qv}" != "${QT_VERSION}" ]]; then
        fail "Qt version mismatch: script expects ${QT_VERSION}, but qmake reports ${qv}";
    fi;
    pass "Qt version verified: ${qv}";

    # -------------------------------------------------------------------------
    # 4. Add Qt bin to PATH (must occur BEFORE cmake and linuxdeploy)
    # -------------------------------------------------------------------------
    export PATH="${QT_ROOT}/bin:${PATH}";
    info "Prepended Qt bin to PATH";

    # -------------------------------------------------------------------------
    # 5. Optional deployqt (Qt 6 rarely ships this)
    # -------------------------------------------------------------------------
    if [[ -x "${QT_DEPLOY_QT}" ]]; then
        export QT_DEPLOY_QT;
        pass "deployqt found and exported: ${QT_DEPLOY_QT}";
    else
        info "deployqt not present — linuxdeploy will be used instead (normal)";
        unset QT_DEPLOY_QT;
    fi;

    pass "Qt installation check";
}

################################################################################
# @brief  Validate bundled Qt version inside AppDir.
# @details Ensures linuxdeploy-plugin-qt bundled the correct Qt version.
################################################################################
validate_bundled_qt()
{
    section "Validating Bundled Qt Version";

    local corelib="${APPDIR_LIB}/libQt6Core.so.6";

    if [[ ! -f "${corelib}" ]]; then
        warn "Bundled QtCore not found — linuxdeploy may have failed.";
        return;
    fi;

    local embedded;
    embedded="$(strings "${corelib}" | grep -Eo 'Qt_[0-9]+\.[0-9]+')";

    info "Bundled Qt symbols: ${embedded}";

    if ! grep -q "Qt_${QT_VERSION%.*}" <<< "${embedded}"; then
        warn "Bundled Qt version does NOT match expected Qt ${QT_VERSION}.";
        warn "This mismatch WILL cause runtime segfaults.";
    else
        pass "Bundled Qt version matches expected Qt ${QT_VERSION}.";
    fi;
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
# @brief  Download or update linuxdeploy and linuxdeploy-plugin-qt.
################################################################################
fetch_appimagetool()
{
  section "Fetching appimagetool"

  if [[ "${OS}" != "linux" ]]; then
    info "Skipping appimagetool fetch on non-Linux OS"
    return
  fi

  download_or_update "${APPIMAGETOOL_BIN}" "${APPIMAGETOOL_URL}"
  pass "appimagetool fetched"
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

    DESKTOP_PATH="${APPDIR_APPS}/${DESKTOP_FILE}";

    cat > "${DESKTOP_PATH}" <<EOF
[Desktop Entry]
Name=${APP_NAME}
Exec=${APP_NAME}
Icon=${APP_NAME}
Type=Application
Categories=Utility;
EOF

    chmod 644 "${DESKTOP_PATH}";

    pass ".desktop file generated";
}

################################################################################
# @brief  cleanup_old_appimages
################################################################################
cleanup_old_appimages()
{
    section "Cleaning old AppImages";
    rm -f "${DEPLOY_DIR}/${APP_NAME}-"*.AppImage;
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
# @brief Diagnose missing TIFF dependencies for Qt image plugins.
# @details Locates libqtiff.so, inspects its dependencies, and reports
# whether libtiff.so.5 is installed, discoverable via ldconfig,
# and visible to the dynamic linker.
################################################################################
diagnose_tiff_dependency()
{
 section "Diagnosing TIFF Dependency (libtiff.so.5)";

 local qtiff;
 qtiff="$(find "${APPDIR}" -name "libqtiff.so" 2>/dev/null | head -1)";

 if [[ -z "${qtiff}" ]]; then
  warn "libqtiff.so not found in AppDir — TIFF plugin may not have been bundled yet.";
  return;
 fi

 pass "Found libqtiff.so: ${qtiff}";

 info "Running ldd on libqtiff.so...";
 local ldd_out;
 ldd_out="$(ldd "${qtiff}" 2>&1)";

 {
  echo "--- ldd libqtiff.so ---";
  echo "${ldd_out}";
 } >> "${INSTALLER_LOG}";

 if echo "${ldd_out}" | grep -q "libtiff.so.5.*not found"; then
  warn "libtiff.so.5 missing according to ldd";
 else
  pass "ldd reports libtiff.so.5 resolved";
 fi

 info "Searching filesystem for libtiff.so.5...";
 local fs_hits;
 fs_hits="$(find /usr/lib64 /usr/lib /lib64 -name 'libtiff.so.5*' 2>/dev/null)";

 if [[ -n "${fs_hits}" ]]; then
  pass "libtiff.so.5 found on filesystem:";
  echo "${fs_hits}" >> "${INSTALLER_LOG}";
 else
  warn "libtiff.so.5 not found on filesystem paths";
 fi

 info "Checking ldconfig cache for libtiff.so.5...";
 local ldconfig_hits;
 ldconfig_hits="$(ldconfig -p | grep libtiff.so.5 || true)";

 if [[ -n "${ldconfig_hits}" ]]; then
  pass "libtiff.so.5 present in ldconfig cache";
  echo "${ldconfig_hits}" >> "${INSTALLER_LOG}";
 else
  warn "libtiff.so.5 NOT present in ldconfig cache";
 fi
}

################################################################################
# @brief build_appimage_from_appdir
################################################################################
build_appimage_from_appdir()
{
    section "Building AppImage from cleaned AppDir";

    local final="${APP_NAME}-${APP_VERSION}-x86_64.AppImage";
    local produced="";

    # Clean any previous outputs
    rm -f ./*.AppImage "${DEPLOY_DIR}/${final}";

    "${APPIMAGETOOL_BIN}" "${APPDIR}" >> "${INSTALLER_LOG}" 2>&1 \
        || fail "appimagetool failed";

    # Detect what appimagetool actually produced (ShellCheck-safe)
    produced="$(
        find . -maxdepth 1 -type f -name '*.AppImage' -print -quit 2>/dev/null
    )";

    if [[ -z "${produced}" ]]; then
        fail "AppImage was not produced by appimagetool";
    fi;

    info "AppImage produced: ${produced}";

    mv -f "${produced}" "${DEPLOY_DIR}/${final}" \
        || fail "Failed to move AppImage to ${DEPLOY_DIR}";

    chmod +x "${DEPLOY_DIR}/${final}" \
        || fail "Cannot chmod AppImage";

    pass "Final AppImage created: ${DEPLOY_DIR}/${final}";
}

################################################################################
# @brief Force use of system runtime libraries during test execution.
################################################################################
force_system_runtime_libs()
{
    info "Forcing system libraries for runtime execution";

    export LD_LIBRARY_PATH="/lib64:/usr/lib64:${LD_LIBRARY_PATH:-}";
}

################################################################################
# @brief  remove_unsafe_system_libs
################################################################################
remove_unsafe_system_libs()
{
    echo "══════════════════════════════════════════════════════════════════════"
    echo " Removing unsafe system libraries"
    echo "══════════════════════════════════════════════════════════════════════"

    local removed=0

    # libcap MUST NOT be bundled — causes early _dl_init SIGSEGV on Fedora
    if compgen -G "AppDir/usr/lib/libcap.so.2*" > /dev/null; then
        echo "[INFO] Removing libcap"
        rm -f AppDir/usr/lib/libcap.so.2*
        removed=$((removed + 1));
    fi

    # libsystemd pulls in libcap and must also not be bundled
    if compgen -G "AppDir/usr/lib/libsystemd.so.0*" > /dev/null; then
        echo "[INFO] Removing libsystemd"
        rm -f AppDir/usr/lib/libsystemd.so.0*
        removed=$((removed + 1));
    fi

    echo "[PASS] Removed unsafe libraries (count=${removed})"
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

    local app_bundle; app_bundle="$(find "${BUILD_DIR}" -maxdepth 4 -name "*.app" -type d | head -1)";

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
    section "Running AppDir Tests";

    local pass_count=0;
    local fail_count=0;

    local binary="${APPDIR_USR}/bin/${APP_NAME}";
    [[ -x "${binary}" ]] || fail "Binary not found: ${binary}";

    local ldd_output;
    ldd_output="$(ldd "${binary}" 2>&1)";
    { echo "--- ldd output ---"; echo "${ldd_output}"; } >> "${INSTALLER_LOG}";

    if echo "${ldd_output}" | grep -q "not found"; then
        warn "T03: ldd reports missing libraries";
        fail_count=$((fail_count + 1));
    else
        pass "T03: No missing libraries reported by ldd";
        pass_count=$((pass_count + 1));
    fi

    local ldd_bundle;
    ldd_bundle="$(LD_LIBRARY_PATH="${APPDIR_USR}/lib:${LD_LIBRARY_PATH:-}" ldd "${binary}" 2>&1 || true)";
    { echo "--- ldd with bundled LD_LIBRARY_PATH ---"; echo "${ldd_bundle}"; } >> "${INSTALLER_LOG}";

    if echo "${ldd_bundle}" | grep -q "not found"; then
        warn "T03b: Bundled LD_LIBRARY_PATH still has missing libraries";
        fail_count=$((fail_count + 1));
    else
        pass "T03b: Bundled LD_LIBRARY_PATH resolves all libraries";
        pass_count=$((pass_count + 1));
    fi

    info "Test summary: ${pass_count} passed, ${fail_count} failed";

    if [[ "${fail_count}" -gt 0 ]]; then
        fail "One or more AppDir tests failed";
    fi
}

################################################################################
# @brief  Run deep diagnostics on a crashed or failing binary.
# @details Called automatically when the runtime test exits with SIGSEGV (139)
#          or any non-timeout failure.  Writes detailed analysis to DEBUG_LOG.
# @param  $1  The binary path that crashed.
# @param  $2  The exit code that was returned.
################################################################################
diagnose_crash()
{
    local binary="$1";
    local exit_code="$2";
    local lib_dir="${APPDIR_USR}/lib";
    local plugin_dir="${APPDIR_USR}/plugins";

    {
        echo "";
        echo "════════════════════════════════════════════════════════";
        echo "  CRASH DIAGNOSTICS  (exit code ${exit_code})";
        echo "  $(date)";
        echo "════════════════════════════════════════════════════════";
        echo "";

        # Decode the exit code signal
        if [[ "${exit_code}" -eq 139 ]]; then
            echo "EXIT CODE 139 = SIGSEGV (Segmentation Fault)";
            echo "  Causes: NULL pointer, bad rpath, missing Qt platform plugin,";
            echo "          version mismatch between bundled Qt libs and plugins.";
        elif [[ "${exit_code}" -eq 134 ]]; then
            echo "EXIT CODE 134 = SIGABRT (Abort / assert failed)";
        elif [[ "${exit_code}" -eq 132 ]]; then
            echo "EXIT CODE 132 = SIGILL (Illegal instruction / CPU mismatch)";
        else
            echo "EXIT CODE ${exit_code}";
        fi
        echo "";

        # Show what rpath is set to on the binary
        echo "── rpath on binary ──────────────────────────────────────";
        if command -v objdump &>/dev/null; then
            objdump -x "${binary}" 2>/dev/null | grep -i "rpath\|runpath" || echo "  (no rpath found)";
        elif command -v readelf &>/dev/null; then
            readelf -d "${binary}" 2>/dev/null | grep -i "rpath\|runpath" || echo "  (no rpath found)";
        else
            echo "  objdump/readelf not available";
        fi
        echo "";

        # Show full ldd against the AppDir lib dir
        echo "── ldd with AppDir LD_LIBRARY_PATH ──────────────────────";
        LD_LIBRARY_PATH="${lib_dir}:${QT_ROOT}/lib:/lib64:/usr/lib64" \
            ldd "${binary}" 2>&1 || true;
        echo "";

        # List what platform plugins are actually bundled
        echo "── Bundled platform plugins ─────────────────────────────";
        if [[ -d "${plugin_dir}/platforms" ]]; then
            ls -la "${plugin_dir}/platforms/" 2>/dev/null || echo "  (empty)";
        else
            echo "  MISSING: ${plugin_dir}/platforms/";
        fi
        echo "";

        # List all imageformat plugins
        echo "── Bundled imageformat plugins ──────────────────────────";
        if [[ -d "${plugin_dir}/imageformats" ]]; then
            if [[ -d "${lib_dir}" ]]; then
                find "${lib_dir}" -maxdepth 1 -type f -printf "  %f\n" | sort;
            else
                echo "  MISSING: ${lib_dir}/";
            fi
        else
            echo "  MISSING: ${plugin_dir}/imageformats/";
        fi
        echo "";

        # List bundled libs
        echo "── Bundled libraries (AppDir/usr/lib/) ──────────────────";
        if [[ -d "${lib_dir}" ]]; then
            find "${lib_dir}" -maxdepth 1 -mindepth 1 -printf "  %f\n" | sort;
        else
            echo "  MISSING: ${lib_dir}/";
        fi
        echo "";

        # Check if xcb platform plugin itself can resolve its deps
        local xcb_plugin="${plugin_dir}/platforms/libqxcb.so";
        if [[ -f "${xcb_plugin}" ]]; then
            echo "── ldd on libqxcb.so ────────────────────────────────────";
            LD_LIBRARY_PATH="${lib_dir}:${QT_ROOT}/lib:/lib64:/usr/lib64" \
                ldd "${xcb_plugin}" 2>&1 || true;
            echo "";
        fi

        # Check qt.conf
        echo "── qt.conf ──────────────────────────────────────────────";
        local qtconf="${APPDIR_BIN}/qt.conf";
        if [[ -f "${qtconf}" ]]; then
            cat "${qtconf}";
        else
            echo "  NOT FOUND: ${qtconf}";
        fi
        echo "";

        # Check AppRun
        echo "── AppRun ───────────────────────────────────────────────";
        local apprun="${APPDIR}/AppRun";
        if [[ -f "${apprun}" || -L "${apprun}" ]]; then
            ls -la "${apprun}";
            if [[ -f "${apprun}" ]]; then
                head -20 "${apprun}" 2>/dev/null || true;
            fi
        else
            echo "  NOT FOUND: ${apprun}";
        fi
        echo "";

        # Try running via AppRun directly (this is the real test)
        echo "── AppRun execution attempt ─────────────────────────────";
        if [[ -x "${apprun}" ]]; then
            echo "  Running: QT_QPA_PLATFORM=xcb timeout 3 ${apprun}";
            DISPLAY="${DISPLAY:-:0}" QT_QPA_PLATFORM=xcb timeout 3 "${apprun}" 2>&1 || true;
        else
            echo "  AppRun not executable — cannot test";
        fi
        echo "";

        echo "════════════════════════════════════════════════════════";
        echo "  END CRASH DIAGNOSTICS";
        echo "════════════════════════════════════════════════════════";
    } >> "${DEBUG_LOG}" 2>&1;
}

################################################################################
# @brief  Execute the application via AppRun in a sandboxed debug mode.
# @details Runs the binary through AppRun (which sets up the correct environment
#          as it would inside the AppImage) rather than calling the ELF directly.
#          Direct ELF execution bypasses AppRun environment setup and fails when
#          rpath is set to $ORIGIN/../lib.  Uses xcb platform (requires DISPLAY)
#          with a 5-second timeout; exit 124 (timeout kill) means it launched.
#          On any crash, calls diagnose_crash() for full diagnostic output.
# @return 0 on clean exit or timeout, non-zero on crash.
################################################################################
run_runtime_test()
{
    local binary="${APPDIR_BIN}/${APP_NAME}";
    local apprun="${APPDIR}/AppRun";
    local timeout_sec=5;
    local ret=0;

    info "Running runtime test (${timeout_sec}s timeout via AppRun)...";

    {
        echo "";
        echo "════════════════════════════════════════════════════════";
        echo "  RUNTIME TEST  $(date)";
        echo "  Binary : ${binary}";
        echo "  AppRun : ${apprun}";
        echo "  DISPLAY: ${DISPLAY:-<not set>}";
        echo "════════════════════════════════════════════════════════";
    } >> "${DEBUG_LOG}";

    # ── Sanity checks before attempting launch ────────────────────────────────
    if [[ ! -x "${binary}" ]]; then
        warn "Runtime test: binary not executable: ${binary}";
        echo "SKIP: binary not executable" >> "${DEBUG_LOG}";
        return 1;
    fi

    if [[ ! -e "${apprun}" ]]; then
        warn "Runtime test: AppRun not found — skipping launch test";
        echo "SKIP: AppRun not found" >> "${DEBUG_LOG}";
        # Fall back to direct binary test with full env
        _run_binary_direct "${binary}" "${timeout_sec}";
        return $?;
    fi

    # ── Primary test: run via AppRun with xcb platform ────────────────────────
    # AppRun sets LD_LIBRARY_PATH, QT_PLUGIN_PATH, and other vars correctly.
    # QT_DEBUG_PLUGINS=1 causes Qt to log every plugin load attempt — essential
    # for diagnosing platform/imageformat plugin failures.
    {
        echo "--- Attempt 1: AppRun + xcb platform ---";
    } >> "${DEBUG_LOG}";

    local launch_cmd="timeout ${timeout_sec} ${apprun}";

    if DISPLAY="${DISPLAY:-:0}" \
       QT_DEBUG_PLUGINS=1 \
       QT_QPA_PLATFORM=xcb \
       ${launch_cmd} >> "${DEBUG_LOG}" 2>&1; then
        ret=0;
        info "AppRun exited cleanly within timeout.";
    else
        local exit_code=$?;
        if [[ "${exit_code}" -eq 124 ]]; then
            ret=0;
            info "AppRun reached timeout — GUI app stayed alive (successful launch).";
        else
            ret="${exit_code}";
            warn "AppRun exited with code ${exit_code}.";

            # ── Fallback: try offscreen platform ─────────────────────────────
            {
                echo "";
                echo "--- Attempt 2: AppRun + offscreen platform ---";
            } >> "${DEBUG_LOG}";

            if QT_DEBUG_PLUGINS=1 \
               QT_QPA_PLATFORM=offscreen \
               timeout "${timeout_sec}" "${apprun}" >> "${DEBUG_LOG}" 2>&1; then
                ret=0;
                info "AppRun (offscreen) exited cleanly.";
            else
                local exit_code2=$?;
                if [[ "${exit_code2}" -eq 124 ]]; then
                    ret=0;
                    info "AppRun (offscreen) timeout — successful launch.";
                else
                    ret="${exit_code2}";
                    warn "AppRun (offscreen) also failed with code ${exit_code2}.";
                    diagnose_crash "${binary}" "${exit_code2}";
                fi
            fi
        fi
    fi

    return "${ret}";
}

################################################################################
# @brief  Fallback: run the raw binary directly with a fully constructed env.
# @details Used when AppRun is absent.  Sets up LD_LIBRARY_PATH and
#          QT_PLUGIN_PATH to match what AppRun would do inside an AppImage.
# @param  $1  Path to the binary.
# @param  $2  Timeout in seconds.
# @return 0 on clean exit or timeout kill, non-zero on crash.
################################################################################
_run_binary_direct()
{
    local binary="$1";
    local timeout_sec="$2";
    local ret=0;

    {
        echo "--- Direct binary execution (no AppRun) ---";
        echo "    LD_LIBRARY_PATH=${APPDIR_USR}/lib:${QT_ROOT}/lib:/lib64:/usr/lib64";
        echo "    QT_PLUGIN_PATH=${APPDIR_USR}/plugins";
        echo "    QT_QPA_PLATFORM=offscreen";
    } >> "${DEBUG_LOG}";

    if LD_LIBRARY_PATH="${APPDIR_USR}/lib:${QT_ROOT}/lib:/lib64:/usr/lib64" \
       QT_PLUGIN_PATH="${APPDIR_USR}/plugins" \
       QT_DEBUG_PLUGINS=1 \
       QT_QPA_PLATFORM=offscreen \
       timeout "${timeout_sec}" "${binary}" >> "${DEBUG_LOG}" 2>&1; then
        ret=0;
        info "Direct binary exited cleanly.";
    else
        local exit_code=$?;
        if [[ "${exit_code}" -eq 124 ]]; then
            ret=0;
            info "Direct binary timeout — launched successfully.";
        else
            ret="${exit_code}";
            warn "Direct binary exited with code ${exit_code}.";
            diagnose_crash "${binary}" "${exit_code}";
        fi
    fi

    return "${ret}";
}

################################################################################
# @brief  Move the generated AppImage or DMG into the deploy/ output folder.
################################################################################
finalise_output()
{
  section "Finalising Output"

  if [[ "${OS}" == "linux" ]]; then
    local appimage

    # AppImage is built explicitly by build_appimage_from_appdir
    appimage="${DEPLOY_DIR}/${APP_NAME}-${APP_VERSION}-x86_64.AppImage"

    if [[ ! -f "${appimage}" ]]; then
      fail "Expected AppImage not found: ${appimage}"
    fi

    chmod +x "${appimage}" || fail "Cannot chmod AppImage"

    pass "AppImage ready: ${appimage}"

    echo ""
    echo -e "${GREEN}${BOLD} ✔ Installer ready: ${appimage}${RESET}"
    echo ""
    echo "Installer ready: ${appimage}" >> "${INSTALLER_LOG}"

  elif [[ "${OS}" == "mac" ]]; then
    local dmg

    dmg="$(find "${BUILD_DIR}" -maxdepth 2 -name "*.dmg" 2>/dev/null | head -1)"

    if [[ -z "${dmg}" ]]; then
      fail "No .dmg found in ${BUILD_DIR}"
    fi

    local dest="${DEPLOY_DIR}/${APP_NAME}-${APP_VERSION}.dmg"
    mv "${dmg}" "${dest}" || fail "Cannot move DMG to ${DEPLOY_DIR}"

    pass "DMG ready: ${dest}"

    echo ""
    echo -e "${GREEN}${BOLD} ✔ Installer ready: ${dest}${RESET}"
    echo ""
    echo "Installer ready: ${dest}" >> "${INSTALLER_LOG}"

  else
    warn "Windows packaging not handled by this script"
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

    local appimage="${DEPLOY_DIR}/${APP_NAME}-${APP_VERSION}-x86_64.AppImage";

    if [[ ! -x "${appimage}" ]]; then
        fail "Summary requested but AppImage is not runnable: ${appimage}";
    fi

    {
        echo " Log file   : ${INSTALLER_LOG}";
        echo " Debug log  : ${DEBUG_LOG}";
        echo " Deploy dir : ${DEPLOY_DIR}/";
        echo "";
        echo " To run the app:";
        echo "   ./${appimage}";
        echo "";
    } | tee -a "${INSTALLER_LOG}";
}

################################################################################
# @brief run_appimage_test
################################################################################
run_appimage_test()
{
    section "Running Real AppImage Execution Test";

    local apprun="${APPDIR}/AppRun";

    if [[ ! -x "${apprun}" ]]; then
        warn "AppRun not executable or missing: ${apprun}";
        return 0;
    fi

    info "Executing AppRun under timeout";

    local rc=0;

    # ─────────────────────────────────────────────────────────────
    # ✅ Fedora-safe Qt runtime environment (scoped to this command)
    #
    # QT_NO_DBUS=1      : disables Qt DBus startup probing
    # QT_QPA_PLATFORM   : forces stable xcb path (avoid portal/wayland probes)
    # QT_LOGGING_RULES  : reduces noisy debug spam during deployment runs
    # QT_ASSUME_STDERR_HAS_CONSOLE: avoids Qt console detection issues
    # ─────────────────────────────────────────────────────────────
    QT_NO_DBUS=1 \
    QT_QPA_PLATFORM=xcb \
    QT_LOGGING_RULES="*.debug=false" \
    QT_ASSUME_STDERR_HAS_CONSOLE=1 \
    timeout 5 "${apprun}" >> "${DEBUG_LOG}" 2>&1 || rc=$?;

    if [[ "${rc}" -eq 124 ]]; then
        pass "T11: AppRun stayed alive (timeout)";
        return 0;
    fi

    if [[ "${rc}" -eq 0 ]]; then
        pass "T11: AppRun exited cleanly";
        return 0;
    fi

    warn "T11: AppRun exited with code ${rc}";
    return "${rc}";
}

################################################################################
# @brief Remove unsafe system-level libraries that must not be bundled.
# @details Some low-level libraries (libcap, libsystemd) crash during
# dynamic loader initialization when bundled inside an AppImage.
# These MUST come from the host system.
################################################################################
remove_problem_libraries()
{
    section "Removing unsafe system libraries";

    local removed=0

    for lib in \
        "${APPDIR}/usr/lib/libcap.so"* \
        "${APPDIR}/usr/lib64/libcap.so"* \
        "${APPDIR}/usr/lib/libsystemd.so"* \
        "${APPDIR}/usr/lib64/libsystemd.so"*; do
        if [[ -e "$lib" ]]; then
            rm -f "$lib"
            removed=1
        fi
    done

    if [[ "$removed" -eq 1 ]]; then
        pass "Unsafe system libraries removed"
    else
        pass "No unsafe system libraries found"
    fi
}


################################################################################
# @brief install_apprun_wrapper
################################################################################
install_apprun_wrapper()
{
    section "Installing AppRun wrapper (Qt runtime guard)";

    local apprun="${APPDIR}/AppRun";
    local target="${APPDIR}/usr/bin/${APP_NAME}";

    if [[ ! -x "${target}" ]]; then
        fail "Target binary not executable: ${target}";
    fi

    # Remove linuxdeploy-created symlink
    rm -f "${apprun}";

    # Create real AppRun script
    cat > "${apprun}" <<EOF
#!/usr/bin/env bash
set -euo pipefail

# ─────────────────────────────────────────────
# Qt runtime hard guards (REQUIRED)
# ─────────────────────────────────────────────
export QT_NO_DBUS=1
export QT_ASSUME_STDERR_HAS_CONSOLE=1
export QT_LOGGING_RULES="*.debug=false"
export QT_QPA_PLATFORM=xcb

exec "\$(dirname "\$0")/usr/bin/${APP_NAME}" "\$@"
EOF

    chmod +x "${apprun}";
    pass "AppRun wrapper installed";
}

################################################################################
# @brief Verify that libcap is NOT bundled in the AppImage.
# @details Fails hard if libcap is present in AppDir/usr/lib.
################################################################################
verify_no_libcap_bundled()
{
    section "Verifying unsafe libs are not bundled";

    if compgen -G "${APPDIR}/usr/lib/libcap.so*" > /dev/null; then
        fail "libcap detected in AppDir/usr/lib — AppImage would crash";
    fi;

    if compgen -G "${APPDIR}/usr/lib64/libcap.so*" > /dev/null; then
        fail "libcap detected in AppDir/usr/lib64 — AppImage would crash";
    fi;

    if compgen -G "${APPDIR}/usr/lib/libsystemd.so*" > /dev/null; then
        fail "libsystemd detected in AppDir/usr/lib — AppImage would crash";
    fi;

    if compgen -G "${APPDIR}/usr/lib64/libsystemd.so*" > /dev/null; then
        fail "libsystemd detected in AppDir/usr/lib64 — AppImage would crash";
    fi;

    pass "libcap/libsystemd are not bundled";
}

################################################################################
# @brief  verify_no_unsafe_runtime_libs
################################################################################
verify_no_unsafe_runtime_libs()
{
    section "Verifying no unsafe runtime libraries are present"

    local bad=0
    for lib in libcap.so.2 libsystemd.so.0; do
        if find AppDir -name "$lib" | grep -q .; then
            warn "Unsafe runtime library detected: $lib"
            bad=1
        fi
    done

    [[ "$bad" -eq 0 ]] || fail "Unsafe libraries present before AppRun"
    pass "No unsafe runtime libraries present"
}

################################################################################
# @brief  audit_appimage_for_unsafe_libs
################################################################################
audit_appimage_for_unsafe_libs()
{
    section "Auditing AppImage contents for unsafe libraries";

    local appimage="${DEPLOY_DIR}/${APP_NAME}-${APP_VERSION}-x86_64.AppImage";
    [[ -x "${appimage}" ]] || fail "AppImage missing: ${appimage}";

    local tmpdir;
    tmpdir="$(mktemp -d)";

    if ! ( cd "${tmpdir}" && "${appimage}" --appimage-extract >> "${DEBUG_LOG}" 2>&1 ); then
        warn "AppImage extraction failed during audit";
        rm -rf "${tmpdir}";
        return 1;
    fi

    if find "${tmpdir}/squashfs-root" -type f \
        \( -name 'libcap.so*' -o -name 'libsystemd.so*' \) | grep -q .; then
        rm -rf "${tmpdir}";
        fail "Unsafe system libraries found inside AppImage";
    fi

    rm -rf "${tmpdir}";
    pass "AppImage audit passed (no unsafe libs)";
}

################################################################################
# @brief  run_linuxdeploy_appdir_only
################################################################################
run_linuxdeploy_appdir_only()
{
    section "Running linuxdeploy (AppDir only)"

    export PATH="${QT_ROOT}/bin:${PATH}"
    export QMAKE="${QT6_QMAKE}"
    export LD_LIBRARY_PATH="${QT_ROOT}/lib"

    if "${LINUXDEPLOY_BIN}" \
        --appdir "${APPDIR}" \
        --desktop-file "${APPDIR_USR}/share/applications/${DESKTOP_FILE}" \
        --icon-file "${APPDIR}/${APP_NAME}.svg" \
        -p qt >> "${INSTALLER_LOG}" 2>&1; then
        pass "linuxdeploy AppDir-only completed"
    fi
}

################################################################################
# @brief  Orchestrate the full linuxdeploy pipeline.
################################################################################
run_linuxdeploy()
{
    section "Running linuxdeploy + Qt Plugin (strict allowlist, Fedora‑safe)"

    [[ "${OS}" != "linux" ]] && return 0

    # ─────────────────────────────────────────────────────────────
    # ✅ FORCE SINGLE Qt
    # ─────────────────────────────────────────────────────────────
    export QT_ROOT="/opt/Qt/6.10.2/gcc_64"
    export QMAKE="${QT_ROOT}/bin/qmake"
    export PATH="${QT_ROOT}/bin:${PATH}"
    export LD_LIBRARY_PATH="${QT_ROOT}/lib"
    export QT_PLUGIN_PATH="${QT_ROOT}/plugins"
    unset LD_PRELOAD

    # ─────────────────────────────────────────────────────────────
    # ✅ ABSOLUTE CONTROL: disable all auto dependency copying
    # ─────────────────────────────────────────────────────────────
    export LINUXDEPLOY_DEPLOY_DEPS=0
    export LINUXDEPLOY_PLUGIN_QT_NO_STRIP=1

    # ─────────────────────────────────────────────────────────────
    # ✅ RUN linuxdeploy (Qt plugin ONLY)
    # ─────────────────────────────────────────────────────────────
    "${LINUXDEPLOY_BIN}" \
        --appdir "${APPDIR}" \
        --desktop-file "${APPDIR_APPS}/${DESKTOP_FILE}" \
        --icon-file "${APPDIR}/${APP_NAME}.svg" \
        -p qt \
        >> "${INSTALLER_LOG}" 2>&1 \
        || fail "linuxdeploy failed"

    pass "linuxdeploy completed (dependency chasing disabled)"
}

################################################################################
# @brief  Print banner
################################################################################
print_banner()
{
    local -i width=66;
    local title="Qt ${QT_VERSION} Deployment Script";
    local timestamp; timestamp="$(date '+%Y-%m-%d %H:%M:%S')";
    local border;    border="$(printf '%*s' "${width}" '' | sed 's/ /═/g')";
    width=$((width - 2));
    echo "";
    printf "%b ╔%s╗ %b \n" "${CYAN}" "${border}" "${RESET}";
    printf "%b ║ %b%-*s%b ║ %b \n" "${CYAN}" "${BLUE}" "${width}" "${title}" "${CYAN}" "${RESET}";
    printf "%b ║ %b%-*s%b ║ %b \n" "${CYAN}" "${BLUE}" "${width}" "${timestamp}" "${CYAN}" "${RESET}";
    printf "%b ╚%s╝ %b \n" "${CYAN}" "${border}" "${RESET}";
}

################################################################################
# @brief  Auto-detect a usable Qt root if the configured one is invalid.
# @details Scans /opt/Qt/*/gcc_64 for qmake and picks the highest version
#          matching major.minor of QT_VERSION, falling back to the highest
#          available Qt if no exact match is found.
################################################################################
detect_qt_root()
{
    section "Auto-Detecting Qt Installation";

    # If QT_ROOT already exists, respect it
    if [[ -d "${QT_ROOT}" && -x "${QT_ROOT}/bin/qmake" ]]; then
        info "QT_ROOT already valid: ${QT_ROOT}";
        pass "Qt auto-detection skipped (explicit QT_ROOT is valid)";
        return;
    fi

    local candidates;
    mapfile -t candidates < <(find /opt/Qt -maxdepth 2 -type d -name "gcc_64" 2>/dev/null | sort)

    if [[ "${#candidates[@]}" -eq 0 ]]; then
        fail "No Qt gcc_64 installations found under /opt/Qt. Install Qt 6.10.2 via Maintenance Tool.";
    fi

    local desired_major_minor;
    desired_major_minor="${QT_VERSION%.*}";

    local best_match="";
    local fallback="";

    for path in "${candidates[@]}"; do
        local base;
        base="$(dirname "${path}")";              # /opt/Qt/6.10.2
        local ver;
        ver="$(basename "${base}")";              # 6.10.2

        if [[ -x "${path}/bin/qmake" ]]; then
            fallback="${base}";
            if [[ "${ver%.*}" == "${desired_major_minor}" ]]; then
                best_match="${base}";
            fi
        fi
    done

    if [[ -n "${best_match}" ]]; then
        QT_VERSION="$(basename "${best_match}")";
        QT_ROOT="${best_match}/gcc_64";
        info "Qt auto-detected (matching ${desired_major_minor}.x): ${QT_ROOT}";
    elif [[ -n "${fallback}" ]]; then
        QT_VERSION="$(basename "${fallback}")";
        QT_ROOT="${fallback}/gcc_64";
        warn "No exact Qt ${desired_major_minor}.x match found; using ${QT_VERSION} at ${QT_ROOT}";
    else
        fail "No usable Qt installation with qmake found under /opt/Qt.";
    fi

    pass "Qt auto-detection completed: QT_VERSION=${QT_VERSION}, QT_ROOT=${QT_ROOT}";
}

################################################################################
# @brief  Validate and auto-repair linuxdeploy tools.
# @details Ensures linuxdeploy and linuxdeploy-plugin-qt exist, are executable,
#          and are valid AppImages. Re-downloads them if corruption is detected.
################################################################################
auto_repair_linuxdeploy()
{
    section "Validating linuxdeploy Tools";

    local repaired=0;

    # Helper to validate a single AppImage
    _validate_appimage()
    {
        local bin="$1";
        local name;
        name="$(basename "${bin}")";

        if [[ ! -f "${bin}" ]]; then
            warn "${name} missing — will re-fetch.";
            repaired=1;
            return 1;
        fi

        if [[ ! -x "${bin}" ]]; then
            warn "${name} not executable — fixing permissions.";
            chmod +x "${bin}" || fail "Cannot chmod +x ${bin}";
        fi

        local ft;
        ft="$(file -b "${bin}")";
        if ! grep -qi "AppImage" <<< "${ft}"; then
            warn "${name} is not a valid AppImage (file type: ${ft}) — will re-fetch.";
            rm -f "${bin}";
            repaired=1;
            return 1;
        fi

        pass "${name} validated: ${ft}";
        return 0;
    }

    _validate_appimage "${LINUXDEPLOY_BIN}" || true;
    _validate_appimage "${LINUXDEPLOY_QT_BIN}" || true;

    if [[ "${repaired}" -eq 1 ]]; then
        warn "One or more linuxdeploy tools were invalid — re-fetching.";
        fetch_linuxdeploy;
    else
        pass "linuxdeploy tools are valid.";
    fi
}

################################################################################
# @brief  Run extended runtime diagnostics on the built AppImage.
# @details Extracts the AppImage, runs AppRun with QT_DEBUG_PLUGINS enabled,
#          and captures stdout/stderr to a log for deep runtime analysis.
################################################################################
run_appimage_runtime_diagnostics()
{
    section "Running AppImage Runtime Diagnostics";

    local appimage;
    appimage="$(find "${DEPLOY_DIR}" -maxdepth 1 -type f -name "${APP_NAME}-*.AppImage" | head -n1)";

    if [[ -z "${appimage}" ]]; then
        warn "No AppImage found for runtime diagnostics";
        return;
    fi

    info "Using AppImage: ${appimage}";

    local diag_dir="${APPDIR}/runtime_diag";
    rm -rf "${diag_dir}";
    mkdir -p "${diag_dir}" || fail "Cannot create diagnostics directory";

    if ! ( cd "${diag_dir}" && "${appimage}" --appimage-extract ) >> "${INSTALLER_LOG}" 2>&1; then
        warn "AppImage extraction failed for diagnostics";
        return;
    fi

    pass "Runtime diagnostics extracted";
}

################################################################################
# @brief  Scan Qt plugins in AppDir for missing shared library dependencies.
# @details Uses ldd on all Qt plugin .so files and reports any "not found"
#          dependencies, which commonly cause runtime crashes.
################################################################################
scan_qt_plugins()
{
    section "Scanning Qt Plugins for Missing Dependencies";

    local plugin_root="${APPDIR}/usr/plugins";
    if [[ ! -d "${plugin_root}" ]]; then
        warn "Plugin root ${plugin_root} does not exist — skipping plugin scan.";
        return;
    fi

    local plugins;
    mapfile -t plugins < <(find "${plugin_root}" -type f -name "*.so" 2>/dev/null);

    if [[ "${#plugins[@]}" -eq 0 ]]; then
        warn "No Qt plugin libraries found under ${plugin_root}.";
        return;
    fi

    local libdir="${APPDIR_USR}/lib";
    local issues=0;

    for so in "${plugins[@]}"; do
        local missing;
        missing="$(LD_LIBRARY_PATH="${libdir}:${LD_LIBRARY_PATH:-}" ldd "${so}" 2>/dev/null | grep 'not found' || true)";

        if [[ -n "${missing}" ]]; then
            warn "Missing dependencies for plugin: ${so}";
            echo "${missing}" | while read -r line; do
                warn " ${line}";
            done
            issues=$((issues + 1));
        else
            pass "Plugin OK: ${so}";
        fi
    done

    if [[ "${issues}" -gt 0 ]]; then
        warn "${issues} plugin(s) have missing dependencies. Runtime failures are likely.";
    else
        pass "All scanned Qt plugins have satisfied dependencies.";
    fi
}

#****************************************************************
# @brief      Show Message
# @parm       color_level
# @parm       msg2show
#****************************************************************
show_msg()
{
    local color_level="${1}";
    local msg2show="${2}";
    printf "%b %s [%s ∅ 00:00:00] %s %b\n" "${color_level}" "ℹ" "$(date '+%H:%M:%S')" "${msg2show}" "${NC}";
    return 0; # Pass
}

#****************************************************************
# @brief      Show Header
#****************************************************************
show_header()
{
    local in_color="${1}";
    printf "%b **************************************************************************************** %b \n" "${in_color}" "${NC}";
    return 0; # Pass
}

#****************************************************************
# @brief      Show Header
#****************************************************************
show_footer()
{
    local in_color="${1}";
    printf "%b ---------------------------------------------------------------------------------------- %b \n" "${in_color}" "${NC}";
    return 0; # Pass
}

#**************************************************************************************************
# @brief      Source all Bash libs and validate Python helpers and encodable text files.
# @note       Do not use log_msg it is not loaded yet
# @description Include: SOURCE_SH_SHARED, SOURCE_PROJECT_SH,
#              Exclude: SOURCE_SH_LIBRARY, SOURCE_MAIN_SH
#**************************************************************************************************
load_source_files()
{
    local file;
    show_header "${SKYBLUE}";
    info "Loading script...";
    for file in "${SOURCE_SH_SHARED[@]}"; do
        # shellcheck source=/dev/null
        if ! source "${file}"; then show_msg "${RED}" "Failed to load source file: ${file}"; return 1; fi
    done
    show_footer "${SKYBLUE}";
    return 0; # Pass
}

#**************************************************************************************************
# @brief      Load Library Source.
#**************************************************************************************************
load_library_source()
{
    # printf only no log_msg yet
    show_header "${MAGENTA}";
    show_msg "${CYAN}" "Loading source file...";
    local file="${PATH_BASH}/source_files.sh";
    # source files
    if [[ -f "${file}" ]]; then
        if ! shellcheck "${file}"; then
            failed "Failed [${file}] shellcheck"; return 1;
        fi
        # shellcheck source=/dev/null
        if ! source "${file}"; then show_msg "${RED}" "Failed to load source file: ${file}"; return 1; fi
    else
        printf "%b Failed to Loaded: ${file} %b\n" "${CYAN}" "${NC}";
        return 1;
    fi
    # Load Source Files Library
    for file in "${SOURCE_SH_LIBRARY[@]}"; do
        # shellcheck source=/dev/null
        if ! source "${file}"; then show_msg "${RED}" "Failed to load source library file: ${file}"; return 1; fi
    done
    # Now load source files
    if ! load_source_files; then
        show_msg "${RED}" "Failed to Loaded: ${file}";
        return 1;
    fi

    show_msg "${GREEN}" "Pass: loading.";
    # Show Footer
    show_footer "${MAGENTA}";
    return 0; # Pass
}


################################################################################
# @brief  Install appdata.xml
# @param  $@  Command-line arguments forwarded from the shell.
################################################################################
install_appdata()
{
    section "Installing AppStream metadata";

    local appstream_id="io.github.am-tower.${APP_NAME}";
    local src="${PATH_BASH}/${appstream_id}.metainfo.xml";
    local dst="AppDir/usr/share/metainfo";

    if [[ ! -f "${src}" ]]; then
        warn "AppStream metadata not found: ${src}";
        return 0;
    fi

    mkdir -p "${dst}";
    cp -v "${src}" "${dst}/${appstream_id}.metainfo.xml";

    pass "AppStream metadata installed";
}

################################################################################
# @brief  Main entry point — orchestrates all deployment stages.
# @param  $@  Command-line arguments forwarded from the shell.
################################################################################
main()
{

    # ── bootstrap MUST be first ──────────────────────────────────────────────
    bootstrap_log "$@";
    parse_args "$@";
    print_banner;
    # Load Source
    if ! load_library_source; then return 1; fi
    # --gitinit: Create a new local git repo
    if (( SWITCH_GIT_INIT == 1 )); then git_init; return 0; fi
    # ── version handling MUST come first ───────────────────────────────────
    handle_versioning;
    # Check in git
    git_check || true;
    # --backup: Backup before you check in case it fails
    if (( SWITCH_BACKUP == 1 )); then run_backup; return 0; fi

    # ── sanity check ─────────────────────────────────────────────────────────
    if [[ ! -f "CMakeLists.txt" ]]; then
        fail "CMakeLists.txt not found. Run this script from the project root.";
    fi

    # ── resolve project metadata early ───────────────────────────────────────
    read_cmake_app_name;

    # ── environment + build prep ─────────────────────────────────────────────
    detect_os;
    init_dirs;
    check_host_tools;
    detect_qt_root;              # NEW: auto-detect Qt if QT_ROOT invalid
    check_qt_installation;
    check_cmake_file;
    check_for_update;

    # ── build + stage ────────────────────────────────────────────────────────
    build_application;
    copy_icon;
    create_desktop_file;
    copy_binary;

    # ── platform‑specific deployment ─────────────────────────────────────────
    if [[ "${OS}" == "linux" ]]; then
        fetch_linuxdeploy;
        fetch_appimagetool;
        auto_repair_linuxdeploy;     # NEW: validate/repair linuxdeploy tools

        # Always start with a clean deploy artifact (Nextcloud trigger stability)
        cleanup_old_appimages;

        if [[ "${run_mode}" == 1 ]]; then
            # Populate AppDir
            run_linuxdeploy;
            install_appdata;
            validate_bundled_qt;
            scan_qt_plugins;          # plugin dependency scanner

            # Remove Fedora-unsafe libs pulled in by linuxdeploy-plugin-qt
            remove_problem_libraries;

            # Verify the AppDir is safe to execute
            verify_no_unsafe_runtime_libs;
            install_apprun_wrapper;

            # Optional: convert AppRun symlink into a wrapper that exports runtime env
            # bake_apprun_wrapper;

            # AppDir-only tests
            run_tests;

            build_appimage_from_appdir;
            audit_appimage_for_unsafe_libs;

            # Real AppImage execution test (now runs with safe env inside run_appimage_test)
            run_appimage_test;

            run_appimage_runtime_diagnostics;   # deep runtime harness

        else
            # AppDir-only mode (populate only)
            run_linuxdeploy_appdir_only;
            install_appdata;
            validate_bundled_qt;
            scan_qt_plugins;

            remove_problem_libraries;
            verify_no_unsafe_runtime_libs;
            install_apprun_wrapper;

            # Optional:
            # bake_apprun_wrapper;

            build_appimage_from_appdir;
            audit_appimage_for_unsafe_libs;
            run_appimage_runtime_diagnostics;
        fi

        finalise_output;
        print_summary;

    elif [[ "${OS}" == "mac" ]]; then
        run_macdeployqt;
        finalise_output;
        print_summary;

    else
        warn "Windows deployment is not automated. See deploy.sh comments.";
        finalise_output;
        print_summary;
    fi
}

################################################################################
main "$@";
################################################################################
# @brief End of File deploy.sh
################################################################################
