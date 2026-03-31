# --------------------------------------------------------------------------------------------------
# CMake
# @file cmake/PackSource.cmake
# @brief Create a single packaged source file for AI review
# @authors Jeffrey Scott Flesher with the help of AI: Copilot
# @date 2026-03-06
# --------------------------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.25)

# --------------------------------------------------------------------------------
# Validate input
# --------------------------------------------------------------------------------
if(NOT DEFINED PROJECT_ROOT)
  message(FATAL_ERROR "PROJECT_ROOT not set. Call with -DPROJECT_ROOT=...")
endif()

file(TO_CMAKE_PATH "${PROJECT_ROOT}" PROJECT_ROOT)

set(OUT_FILE "${PROJECT_ROOT}/Project-Source.txt")
file(WRITE "${OUT_FILE}" "")

# --------------------------------------------------------------------------------
# Defaults (if exporter did not pass values)
# --------------------------------------------------------------------------------
if(NOT DEFINED INCLUDE_EXTENSIONS OR INCLUDE_EXTENSIONS STREQUAL "")
  set(INCLUDE_EXTENSIONS cpp h md)
endif()

if(NOT DEFINED EXCLUDE_DIRS OR EXCLUDE_DIRS STREQUAL "")
  set(EXCLUDE_DIRS
    build*
    .rcc
    CMakeFiles
    .git
    .qtcreator
    engines
    0-Archive
  )
endif()

separate_arguments(INCLUDE_EXTENSIONS)
separate_arguments(EXCLUDE_DIRS)

# Safety: ensure build* exists in the exclude list so build-wsl is excluded.
list(FIND EXCLUDE_DIRS "build*" _build_star_idx)
if(_build_star_idx EQUAL -1)
  list(APPEND EXCLUDE_DIRS "build*")
endif()

# --------------------------------------------------------------------------------
# Helpers: formatting
# --------------------------------------------------------------------------------
function(psd_divider out_file)
  file(APPEND "${out_file}" "===================================================================================================================================\n")
endfunction()

function(psd_file_header out_file rel_path)
  psd_divider("${out_file}")
  file(APPEND "${out_file}" "=============================== FILE: ${rel_path} ===============================\n")
  psd_divider("${out_file}")
endfunction()

# --------------------------------------------------------------------------------
# Helpers: tree detection + tree output
# --------------------------------------------------------------------------------
function(psd_have_tree out_var)
  execute_process(
    COMMAND bash -lc "command -v tree >/dev/null 2>&1"
    RESULT_VARIABLE _rv
    OUTPUT_QUIET
    ERROR_QUIET
  )
  if(_rv EQUAL 0)
    set(${out_var} TRUE PARENT_SCOPE)
  else()
    set(${out_var} FALSE PARENT_SCOPE)
  endif()
endfunction()

# Build tree ignore string from EXCLUDE_DIRS.
# tree -I supports wildcard patterns separated by '|'
function(psd_build_tree_ignore out_var)
  set(_ignore "Archive") # keep your original Archive exclusion
  foreach(p IN LISTS EXCLUDE_DIRS)
    list(APPEND _ignore "${p}")
  endforeach()
  list(REMOVE_DUPLICATES _ignore)
  list(JOIN _ignore "|" _joined)
  set(${out_var} "${_joined}" PARENT_SCOPE)
endfunction()

function(psd_write_project_tree out_file project_root)
  psd_have_tree(_have_tree)

  psd_divider("${out_file}")
  file(APPEND "${out_file}" "==================================== PROJECT TREE ====================================\n")
  psd_divider("${out_file}")

  if(_have_tree)
    psd_build_tree_ignore(_tree_ignore)

    # IMPORTANT: ignore patterns come from EXCLUDE_DIRS (includes build* so build-wsl is excluded)
    execute_process(
      COMMAND bash -lc "cd '${project_root}' && tree -a -I '${_tree_ignore}'"
      OUTPUT_VARIABLE _tree_out
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_VARIABLE _tree_err
      RESULT_VARIABLE _tree_rv
    )

    if(_tree_rv EQUAL 0)
      file(APPEND "${out_file}" "${_tree_out}\n\n")
    else()
      file(APPEND "${out_file}" "tree execution failed (rv=${_tree_rv}).\n${_tree_err}\n\n")
    endif()
  else()
    file(APPEND "${out_file}" "tree is not installed. (Optional)\n\n")
  endif()
endfunction()

# --------------------------------------------------------------------------------
# Helper: append a file by absolute path (used for CMakeLists.txt injection)
# --------------------------------------------------------------------------------
function(psd_append_file_abs out_file abs_path rel_label)
  if(EXISTS "${abs_path}")
    psd_file_header("${out_file}" "${rel_label}")
    file(READ "${abs_path}" _content)
    file(APPEND "${out_file}" "${_content}\n\n")
  endif()
endfunction()

# --------------------------------------------------------------------------------
# File patterns to include (driven by INCLUDE_EXTENSIONS)
# --------------------------------------------------------------------------------
set(PATTERNS "")
foreach(ext IN LISTS INCLUDE_EXTENSIONS)
  string(REGEX REPLACE "^\\." "" _ext "${ext}")
  list(APPEND PATTERNS "*.${_ext}")
endforeach()

# --------------------------------------------------------------------------------
# Collect candidate files (RAW, UNTRUSTED LIST)
# Note: CONFIGURE_DEPENDS is NOT allowed in script mode (-P)
# --------------------------------------------------------------------------------
set(ALL_FILES "")
foreach(p IN LISTS PATTERNS)
  file(GLOB_RECURSE TMP
       RELATIVE "${PROJECT_ROOT}"
       "${PROJECT_ROOT}/${p}")
  list(APPEND ALL_FILES ${TMP})
endforeach()

# --------------------------------------------------------------------------------
# HARD EXCLUSIONS (single authoritative pass)
# Convert EXCLUDE_DIRS into robust path-segment filters.
# - For prefix patterns like build*: exclude any dir segment starting with "build"
# - For exact patterns like engines: exclude any dir segment equal to "engines"
# --------------------------------------------------------------------------------
foreach(pattern IN LISTS EXCLUDE_DIRS)
  if(pattern MATCHES "\\*$")
    string(REPLACE "*" "" prefix "${pattern}")

    # Escape regex metacharacters we might realistically see.
    string(REPLACE "." "\\\\." prefix_esc "${prefix}")
    string(REPLACE "-" "\\\\-" prefix_esc "${prefix_esc}")

    # Exclude anywhere in the path: (^|/)prefix[^/]*/
    list(FILTER ALL_FILES EXCLUDE REGEX "(^|/)${prefix_esc}[^/]*/")
  else()
    set(exact "${pattern}")
    string(REPLACE "." "\\\\." exact_esc "${exact}")
    string(REPLACE "-" "\\\\-" exact_esc "${exact_esc}")

    # Exclude anywhere in the path: (^|/)exact/
    list(FILTER ALL_FILES EXCLUDE REGEX "(^|/)${exact_esc}/")
  endif()
endforeach()

# --------------------------------------------------------------------------------
# Exclude output file itself
# --------------------------------------------------------------------------------
list(REMOVE_ITEM ALL_FILES "Project-Source.txt")

# --------------------------------------------------------------------------------
# Normalize list
# --------------------------------------------------------------------------------
list(REMOVE_DUPLICATES ALL_FILES)
list(SORT ALL_FILES)
list(LENGTH ALL_FILES FILE_COUNT)

# --------------------------------------------------------------------------------
# Header
# --------------------------------------------------------------------------------
file(APPEND "${OUT_FILE}" "Project-Source.txt\n")
file(APPEND "${OUT_FILE}" "Generated by CMake script\n")
file(APPEND "${OUT_FILE}" "Root: ${PROJECT_ROOT}\n")
file(APPEND "${OUT_FILE}" "Included extensions: ${INCLUDE_EXTENSIONS}\n")
file(APPEND "${OUT_FILE}" "Excluded dirs: ${EXCLUDE_DIRS}\n")
file(APPEND "${OUT_FILE}" "File count: ${FILE_COUNT}\n\n")

# --------------------------------------------------------------------------------
# Project tree section (runs BEFORE file content so it appears near the top)
# --------------------------------------------------------------------------------
psd_write_project_tree("${OUT_FILE}" "${PROJECT_ROOT}")

# --------------------------------------------------------------------------------
# Always include CMakeLists.txt explicitly (not matched by PATTERNS)
# --------------------------------------------------------------------------------
psd_append_file_abs("${OUT_FILE}" "${PROJECT_ROOT}/CMakeLists.txt" "CMakeLists.txt")

# --------------------------------------------------------------------------------
# FINAL SAFETY NET + WRITE
# --------------------------------------------------------------------------------
foreach(f IN LISTS ALL_FILES)
  set(abs "${PROJECT_ROOT}/${f}")
  if(NOT EXISTS "${abs}")
    continue()
  endif()

  psd_file_header("${OUT_FILE}" "${f}")
  file(READ "${abs}" content)
  file(APPEND "${OUT_FILE}" "${content}\n\n")
endforeach()

message(STATUS "Wrote ${OUT_FILE} (${FILE_COUNT} files)")

# --------------------------------------------------------------------------------------------------
# End of cmake/PackSource.cmake
# --------------------------------------------------------------------------------------------------
