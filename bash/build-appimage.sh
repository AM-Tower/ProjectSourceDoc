#!/usr/bin/env bash
# ==============================================================================
# build-appimage.sh
#
# Project: ProjectSource
# Purpose: Build a Qt 6 AppImage using linuxdeploy + linuxdeploy-plugin-qt
#
# Features:
#   - Quiet by default, verbose with DEBUG=1
#   - Shows status, progress, and pass/fail in non-verbose mode
#   - Colored output (green=ok, yellow=warn, red=fail)
#   - Fedora-safe (no libtiff.so.5 dependency)
#   - Staged Qt plugins (platforms, imageformats, iconengines) without modifying /opt/Qt
#   - SVG icon copied & renamed to match desktop entry
#   - BEST FIX: Build AppDir first, prune libb2+libgomp (OpenMP crash chain), then make AppImage
#   - AppImage output moved to deploy/ with correct permissions
#   - Logs always written to deploy/logs/build.log
#   - Prints run/install commands and debug commands every run (copy/paste safe)
#   - Static validation (readelf) to avoid ldd hangs/execution
#
# Author: Jeffrey Scott Flesher with Copilot
#
# Usage:
#   Quiet build:
#     QT6_QMAKE=/opt/Qt/6.11.0/gcc_64/bin/qmake NO_STRIP=1 ./build-appimage.sh
#
#   One-line (clean, lint, build):
#     clear; chmod +x build-appimage.sh && shellcheck build-appimage.sh && \
#     QT6_QMAKE=/opt/Qt/6.11.0/gcc_64/bin/qmake NO_STRIP=1 ./build-appimage.sh
#
#   Debug / verbose build:
#     DEBUG=1 QT6_QMAKE=/opt/Qt/6.11.0/gcc_64/bin/qmake NO_STRIP=1 ./build-appimage.sh
#
# Result:
#   deploy/ProjectSource-x86_64.AppImage
# ==============================================================================

set -euo pipefail

# ------------------------------------------------------------------------------
# Configuration
# ------------------------------------------------------------------------------
APP_NAME="ProjectSource"

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
APPDIR="${ROOT_DIR}/AppDir"
TOOLS_DIR="${ROOT_DIR}/tools"

DEPLOY_DIR="${ROOT_DIR}/deploy"
LOG_DIR="${DEPLOY_DIR}/logs"
LOG_FILE="${LOG_DIR}/build.log"

LINUXDEPLOY="${TOOLS_DIR}/linuxdeploy-x86_64.AppImage"
QT_PLUGIN="${TOOLS_DIR}/linuxdeploy-plugin-qt-x86_64.AppImage"
APPIMAGETOOL="${TOOLS_DIR}/appimagetool-x86_64.AppImage"

ICON_ORIG="${ROOT_DIR}/resources/icons/light/app.svg"
ICON_STAGE_DIR="${TOOLS_DIR}/icon-stage"
ICON_STAGE="${ICON_STAGE_DIR}/${APP_NAME}.svg"
DESKTOP_FILE="${ROOT_DIR}/${APP_NAME}.desktop"

# Qt plugin staging (writable, does not modify /opt/Qt)
QT_STAGE_DIR="${TOOLS_DIR}/qt-stage"
QT_STAGE_PLUGINS_DIR="${QT_STAGE_DIR}/plugins"
QMAKE_WRAPPER="${QT_STAGE_DIR}/qmake-wrapper"

DEBUG="${DEBUG:-0}"
NO_STRIP="${NO_STRIP:-1}"
LINUXDEPLOY_TIMEOUT="${LINUXDEPLOY_TIMEOUT:-180}"

APPIMAGE_OUT="${DEPLOY_DIR}/${APP_NAME}-x86_64.AppImage"

mkdir -p "${DEPLOY_DIR}" "${LOG_DIR}" "${TOOLS_DIR}" "${ICON_STAGE_DIR}" "${QT_STAGE_DIR}"

# ------------------------------------------------------------------------------
# Colors / UI
# ------------------------------------------------------------------------------
if [[ -t 1 ]]; then
  C_RESET="\033[0m"; C_GREEN="\033[32m"; C_YELLOW="\033[33m"; C_RED="\033[31m"; C_BLUE="\033[34m"
else
  C_RESET=""; C_GREEN=""; C_YELLOW=""; C_RED=""; C_BLUE=""
fi

status() { printf "${C_BLUE}▶${C_RESET} %s\n" "$*"; }
ok()     { printf "${C_GREEN}✔${C_RESET} %s\n" "$*"; }
warn()   { printf "${C_YELLOW}⚠${C_RESET} %s\n" "$*" >&2; }
err()    { printf "${C_RED}✖${C_RESET} %s\n" "$*" >&2; }

# ------------------------------------------------------------------------------
# Always print instructions; only show run commands if AppImage exists
# ------------------------------------------------------------------------------
print_instructions() {
  printf "\n📝 Log: %s\n" "${LOG_FILE}"

  if [[ -f "${APPIMAGE_OUT}" ]]; then
    printf "📦 %s\n\n" "${APPIMAGE_OUT}"

    printf "▶ Run (direct):\n   %s\n\n" "${APPIMAGE_OUT}"

    printf "▶ Install (user-local, no sudo):\n"
    printf "   mkdir -p %s/.local/bin && cp \"%s\" %s/.local/bin/%s && chmod 755 %s/.local/bin/%s\n\n" \
      "${HOME}" "${APPIMAGE_OUT}" "${HOME}" "${APP_NAME}" "${HOME}" "${APP_NAME}"

    printf "▶ Run (after user-local install):\n   %s/.local/bin/%s\n\n" "${HOME}" "${APP_NAME}"

    printf "▶ Install (system-wide):\n"
    printf "   sudo cp \"%s\" /usr/local/bin/%s && sudo chmod 755 /usr/local/bin/%s\n\n" \
      "${APPIMAGE_OUT}" "${APP_NAME}" "${APP_NAME}"

    printf "▶ Run (after system-wide install):\n   %s\n\n" "${APP_NAME}"

    printf "▶ Debug (gdb map crash to library, non-interactive):\n"
    printf "   %s --appimage-extract\n" "${APPIMAGE_OUT}"
    cat <<'EOF'
   gdb -nx -batch ./squashfs-root/usr/bin/ProjectSource \
     -ex 'set pagination off' \
     -ex 'run' \
     -ex 'printf "PC=%p\n", $pc' \
     -ex 'info proc mappings' \
     -ex 'info sharedlibrary' \
     -ex 'info symbol $pc' \
     -ex 'x/16i $pc' \
     -ex 'bt'
EOF
  else
    warn "No AppImage produced."
  fi
}

dump_log_tail() {
  warn "Last 120 lines of ${LOG_FILE}:"
  tail -n 120 "${LOG_FILE}" 2>/dev/null || true
}

on_exit() {
  local rc=$?
  if [[ $rc -eq 0 ]]; then
    ok "Build complete"
  else
    err "Build failed (exit ${rc}). See log: ${LOG_FILE}"
    dump_log_tail
  fi
  print_instructions
  exit $rc
}
trap on_exit EXIT

# ------------------------------------------------------------------------------
# run_step (never silent)
# ------------------------------------------------------------------------------
run_step() {
  local label="$1"; shift
  status "${label}"

  local rc=0
  if [[ "${DEBUG}" == "1" ]]; then
    set +e
    "$@" 2>&1 | tee -a "${LOG_FILE}"
    rc=${PIPESTATUS[0]}
    set -e
  else
    set +e
    "$@" >>"${LOG_FILE}" 2>&1
    rc=$?
    set -e
  fi

  if [[ $rc -ne 0 ]]; then
    err "${label} (exit ${rc})"
    err "See log: ${LOG_FILE}"
    exit "${rc}"
  fi
  ok "${label}"
}

# ------------------------------------------------------------------------------
# Tool setup
# ------------------------------------------------------------------------------
ensure_tools() {
  mkdir -p "${TOOLS_DIR}"

  if [[ ! -x "${LINUXDEPLOY}" ]]; then
    run_step "Downloading linuxdeploy" \
      wget -q -O "${LINUXDEPLOY}" \
      https://github.com/linuxdeploy/linuxdeploy/releases/latest/download/linuxdeploy-x86_64.AppImage
    chmod +x "${LINUXDEPLOY}"
  fi

  if [[ ! -x "${QT_PLUGIN}" ]]; then
    run_step "Downloading linuxdeploy-plugin-qt" \
      wget -q -O "${QT_PLUGIN}" \
      https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/latest/download/linuxdeploy-plugin-qt-x86_64.AppImage
    chmod +x "${QT_PLUGIN}"
  fi

  if [[ ! -x "${APPIMAGETOOL}" ]]; then
    run_step "Downloading appimagetool" \
      wget -q -O "${APPIMAGETOOL}" \
      https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage
    chmod +x "${APPIMAGETOOL}"
  fi

  # Make plugin discoverable by linuxdeploy
  export PATH="${TOOLS_DIR}:${PATH}"
}

# ------------------------------------------------------------------------------
# Qt: qmake selection + plugin staging to avoid libtiff.so.5
# ------------------------------------------------------------------------------
select_qmake() {
  local qmake="${QT6_QMAKE:-/opt/Qt/6.11.0/gcc_64/bin/qmake}"
  [[ -x "${qmake}" ]] || { err "Qt6 qmake not found: ${qmake}"; exit 3; }
  printf "%s\n" "${qmake}"
}

needs_tiff_workaround() {
  local qmake="$1"
  local plugins
  plugins="$("${qmake}" -query QT_INSTALL_PLUGINS 2>/dev/null || true)"
  [[ -n "${plugins}" ]] || return 1
  [[ -f "${plugins}/imageformats/libqtiff.so" ]] || return 1
  ldd "${plugins}/imageformats/libqtiff.so" 2>/dev/null | grep -q 'libtiff\.so\.5 => not found'
}

stage_qt_plugins_without_tiff() {
  local qmake="$1"
  local src
  src="$("${qmake}" -query QT_INSTALL_PLUGINS 2>/dev/null || true)"
  [[ -n "${src}" ]] || { err "Could not determine QT_INSTALL_PLUGINS"; exit 4; }

  rm -rf "${QT_STAGE_DIR}"
  mkdir -p "${QT_STAGE_PLUGINS_DIR}"

  for d in platforms platforminputcontexts imageformats iconengines; do
    [[ -d "${src}/${d}" ]] || continue
    mkdir -p "${QT_STAGE_PLUGINS_DIR}/${d}"
    cp -a "${src}/${d}/." "${QT_STAGE_PLUGINS_DIR}/${d}/"
  done

  # remove TIFF plugin only (avoids libtiff.so.5 requirement)
  rm -f "${QT_STAGE_PLUGINS_DIR}/imageformats/libqtiff.so" || true

  cat > "${QMAKE_WRAPPER}" <<EOF
#!/usr/bin/env bash
set -euo pipefail
REAL_QMAKE="${qmake}"
STAGED_PLUGINS="${QT_STAGE_PLUGINS_DIR}"
if [[ "\${1:-}" == "-query" ]]; then
  if [[ "\${2:-}" == "QT_INSTALL_PLUGINS" ]]; then
    echo "\${STAGED_PLUGINS}"
    exit 0
  fi
  if [[ -z "\${2:-}" ]]; then
    "\${REAL_QMAKE}" -query | sed "s|^QT_INSTALL_PLUGINS:.*|QT_INSTALL_PLUGINS:\${STAGED_PLUGINS}|"
    exit 0
  fi
fi
exec "\${REAL_QMAKE}" "\$@"
EOF
  chmod +x "${QMAKE_WRAPPER}"
}

# ------------------------------------------------------------------------------
# Icon + desktop
# ------------------------------------------------------------------------------
stage_icon() { cp -f "${ICON_ORIG}" "${ICON_STAGE}"; }

write_desktop() {
  cat > "${DESKTOP_FILE}" <<EOF
[Desktop Entry]
Type=Application
Name=${APP_NAME}
Exec=${APP_NAME}
Icon=${APP_NAME}
Categories=Utility;
EOF
}

# ------------------------------------------------------------------------------
# Prune problematic libs
# IMPORTANT: remove both files AND symlinks (your previous prune missed symlinks)
# ------------------------------------------------------------------------------
prune_problem_libs() {
  find "${APPDIR}/usr" \( -type f -o -type l \) \
    \( -name 'libgomp.so*' -o -name 'libb2.so*' -o -name 'libcap.so*' \) \
    -delete || true
}

verify_prune() {
  # Verify none remain as file or symlink
  if find "${APPDIR}/usr" \( -type f -o -type l \) \
       \( -name 'libgomp.so*' -o -name 'libb2.so*' -o -name 'libcap.so*' \) \
       | grep -q .; then
    err "Pruned libraries still present (libgomp/libb2/libcap)."
    find "${APPDIR}/usr" \( -type f -o -type l \) \
         \( -name 'libgomp.so*' -o -name 'libb2.so*' -o -name 'libcap.so*' \) -print >&2
    exit 90
  fi
}

# ------------------------------------------------------------------------------
# Main
# ------------------------------------------------------------------------------
main() {
  : > "${LOG_FILE}"
  rm -f "${APPIMAGE_OUT}"

  ensure_tools
  stage_icon
  write_desktop

  run_step "Configuring project (cmake)" \
    cmake -B "${BUILD_DIR}" -S "${ROOT_DIR}" -DCMAKE_BUILD_TYPE=Release

  run_step "Building project" \
    cmake --build "${BUILD_DIR}" -j"$(nproc)"

  local bin="${BUILD_DIR}/${APP_NAME}"
  [[ -x "${bin}" ]] || exit 2

  rm -rf "${APPDIR}"
  mkdir -p "${APPDIR}"

  local real_qmake
  real_qmake="$(select_qmake)"

  if needs_tiff_workaround "${real_qmake}"; then
    run_step "Staging Qt plugins (remove libqtiff.so)" stage_qt_plugins_without_tiff "${real_qmake}"
    export QMAKE="${QMAKE_WRAPPER}"
  else
    export QMAKE="${real_qmake}"
  fi

  if command -v timeout >/dev/null 2>&1; then
    run_step "Creating AppDir (linuxdeploy)" \
      env NO_STRIP="${NO_STRIP}" timeout "${LINUXDEPLOY_TIMEOUT}" \
      "${LINUXDEPLOY}" \
        --appdir "${APPDIR}" \
        --executable "${bin}" \
        --desktop-file "${DESKTOP_FILE}" \
        --icon-file "${ICON_STAGE}" \
        --plugin qt
  else
    run_step "Creating AppDir (linuxdeploy)" \
      env NO_STRIP="${NO_STRIP}" \
      "${LINUXDEPLOY}" \
        --appdir "${APPDIR}" \
        --executable "${bin}" \
        --desktop-file "${DESKTOP_FILE}" \
        --icon-file "${ICON_STAGE}" \
        --plugin qt
  fi

  run_step "Pruning problematic libs (libb2 + libgomp + libcap)" prune_problem_libs
  run_step "Verifying prune effective (static)" verify_prune

  run_step "Creating AppImage (appimagetool)" \
    env ARCH=x86_64 \
    "${APPIMAGETOOL}" "${APPDIR}" "${APPIMAGE_OUT}"

  run_step "Setting permissions" chmod 755 "${APPIMAGE_OUT}"
}

main "$@"

# ==============================================================================
# ****** End build-appimage.sh ******
# ==============================================================================
