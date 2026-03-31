#!/usr/bin/env bash

# #############################################################################
# @file    deep-runtime-diagnose.sh
# @brief   Deep runtime diagnostics for Linux / AppImage environments
# @author  Jeffrey Scott Flesher
# @version 2.0
# @date    2026-03-29
#
# Usage:
# clear; chmod +x deep-runtime-diagnose.sh && shellcheck deep-runtime-diagnose.sh && ./deep-runtime-diagnose.sh
# clear; chmod +x deep-runtime-diagnose.sh && shellcheck deep-runtime-diagnose.sh && ./deep-runtime-diagnose.sh AppDir/usr/bin/ProjectSourceDoc
# #############################################################################

set -Eeuo pipefail;

# #############################################################################
# Print basic system runtime information
# #############################################################################
print_system_info()
{
  echo;
  echo "===== SYSTEM =====";
  uname -srmo;
  echo "glibc:";
  ldd --version | head -n1 || true;
  echo "loader:";
  /lib64/ld-linux-x86-64.so.2 --version | head -n1 || true;
}

# #############################################################################
# Print runtime environment variables that affect AppImages
# #############################################################################
print_runtime_env()
{
  echo;
  echo "===== RUNTIME ENV =====";
  echo "LD_LIBRARY_PATH=${LD_LIBRARY_PATH:-<unset>}";
  echo "QT_PLUGIN_PATH=${QT_PLUGIN_PATH:-<unset>}";
  echo "QT_QPA_PLATFORM=${QT_QPA_PLATFORM:-<unset>}";
  echo "NO_STRIP=${NO_STRIP:-<unset>}";
}

# #############################################################################
# Show how the dynamic loader resolves libraries
# #############################################################################
print_loader_resolution()
{
  echo;
  echo "===== LOADER RESOLUTION =====";
  echo "ld-linux path:";
  readlink -f /lib64/ld-linux-x86-64.so.2 || true;
  echo;
  echo "Loader test (--list /bin/true):";
  /lib64/ld-linux-x86-64.so.2 --list /bin/true 2>/dev/null | head -n30 || true;
}

# #############################################################################
# Inspect GLIBC symbol versions exported by the host
# #############################################################################
print_glibc_symbols()
{
  echo;
  echo "===== GLIBC SYMBOL VERSIONS (TAIL) =====";
  if [[ -r /lib64/libc.so.6 ]]; then strings /lib64/libc.so.6 | grep -E '^GLIBC_' | sort -Vu | tail -n15; else echo "Cannot read /lib64/libc.so.6"; fi;
}

# #############################################################################
# Run ldd on a target binary or AppRun
# #############################################################################
probe_binary()
{
  local bin="$1";
  echo;
  echo "===== LDD: ${bin} =====";
  if [[ -x "${bin}" ]]; then ldd "${bin}" || true; else echo "Not executable or not found: ${bin}"; fi;
}

# #############################################################################
# Inspect ELF interpreter and RUNPATH/RPATH
# #############################################################################
inspect_elf()
{
  local bin="$1";
  echo;
  echo "===== ELF INSPECTION: ${bin} =====";
  if [[ -x "${bin}" ]]; then readelf -l "${bin}" | grep -E 'interpreter' || true; readelf -d "${bin}" | grep -E 'RPATH|RUNPATH' || true; else echo "Not executable or not found: ${bin}"; fi;
}

# #############################################################################
# Program entry point
# #############################################################################
main()
{
  local target="${1:-}";
  print_system_info;
  print_runtime_env;
  print_loader_resolution;
  print_glibc_symbols;
  if [[ -n "${target}" ]]; then probe_binary "${target}"; inspect_elf "${target}"; else echo; echo "===== NOTE ====="; echo "No target binary specified."; echo "You may re-run with:"; echo "  ./deep-runtime-diagnose.sh AppDir/AppRun"; echo "  ./deep-runtime-diagnose.sh AppDir/usr/bin/ProjectSourceDoc"; fi;
}

# #############################################################################
# Invoke entry point
# #############################################################################
main "$@";

# #############################################################################
# End of file: deep-runtime-diagnose.sh
# #############################################################################
