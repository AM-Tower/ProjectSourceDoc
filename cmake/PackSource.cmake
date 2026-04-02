# --------------------------------------------------------------------------------------------------
# CMake
# @file cmake/PackSource.cmake
# @brief Create a single packaged source file for AI review
# @authors Jeffrey Scott Flesher with the help of AI: Copilot
# @date 2026-04-01
# --------------------------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.25)

# --------------------------------------------------------------------------------
# Validate input
# --------------------------------------------------------------------------------
if(NOT DEFINED PROJECT_ROOT)
  message(FATAL_ERROR "PROJECT_ROOT not set. Call with -DPROJECT_ROOT=...")
endif()

file(TO_CMAKE_PATH "${PROJECT_ROOT}" PROJECT_ROOT)

# --------------------------------------------------------------------------------
# Debug log (append)
# --------------------------------------------------------------------------------
file(APPEND "${PROJECT_ROOT}/ProjectSource-Debug.log"
  "\n--- PackSource.cmake ---\n"
  "PROJECT_ROOT=${PROJECT_ROOT}\n"
  "INCLUDE_EXTENSIONS=${INCLUDE_EXTENSIONS}\n"
  "EXCLUDE_DIRS=${EXCLUDE_DIRS}\n"
)

# --------------------------------------------------------------------------------
# Output file
# --------------------------------------------------------------------------------
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
    AppDir
  )
endif()

# --------------------------------------------------------------------------------
# Compatibility: accept legacy exporter variable name
# --------------------------------------------------------------------------------
if((NOT DEFINED INCLUDE_EXTENSIONS OR INCLUDE_EXTENSIONS STREQUAL "")
   AND DEFINED INCLUDE_EXTS
   AND NOT INCLUDE_EXTS STREQUAL "")
  set(INCLUDE_EXTENSIONS "${INCLUDE_EXTS}")
endif()

# Safety: ensure build* exists in the exclude list
list(FIND EXCLUDE_DIRS "build*" _build_star_idx)
if(_build_star_idx EQUAL -1)
  list(APPEND EXCLUDE_DIRS "build*")
endif()

# --------------------------------------------------------------------------------
# Helpers: formatting
# --------------------------------------------------------------------------------
function(psd_divider out_file)
  file(APPEND "${out_file}"
       "===================================================================================================================================\n")
endfunction()

function(psd_file_header out_file rel_path)
  psd_divider("${out_file}")
  file(APPEND "${out_file}"
       "=============================== FILE: ${rel_path} ===============================\n")
  psd_divider("${out_file}")
endfunction()

# --------------------------------------------------------------------------------
# Helpers: tree detection + tree output
# --------------------------------------------------------------------------------
function(psd_have_tree out_var)
  execute_process(
    COMMAND tree --version
    RESULT_VARIABLE _rv
    OUTPUT_QUIET
    ERROR_QUIET
  )
  set(${out_var} FALSE PARENT_SCOPE)
  if(_rv EQUAL 0)
    set(${out_var} TRUE PARENT_SCOPE)
  endif()
endfunction()

function(psd_build_tree_ignore out_var)
  set(_ignore "Archive")
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
  file(APPEND "${out_file}"
       "==================================== PROJECT TREE ====================================\n")
  psd_divider("${out_file}")

  if(_have_tree)
    psd_build_tree_ignore(_tree_ignore)

    execute_process(
      COMMAND tree -a --dirsfirst -I "${_tree_ignore}"
      WORKING_DIRECTORY "${project_root}"
      OUTPUT_VARIABLE _tree_out
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_VARIABLE _tree_err
      RESULT_VARIABLE _tree_rv
    )

    file(APPEND "${PROJECT_ROOT}/ProjectSource-Debug.log"
      "TREE available=TRUE\n"
      "TREE result=${_tree_rv}\n"
    )

    if(_tree_rv EQUAL 0)
      file(APPEND "${out_file}" "${_tree_out}\n\n")
    else()
      file(APPEND "${out_file}"
           "tree execution failed (rv=${_tree_rv}).\n${_tree_err}\n\n")
      file(APPEND "${PROJECT_ROOT}/ProjectSource-Debug.log"
           "TREE stderr:\n${_tree_err}\n")
    endif()
  else()
    file(APPEND "${out_file}" "tree is not installed. (Optional)\n\n")
    file(APPEND "${PROJECT_ROOT}/ProjectSource-Debug.log"
         "TREE available=FALSE\n")
  endif()
endfunction()

# --------------------------------------------------------------------------------
# Helper: append file by absolute path
# --------------------------------------------------------------------------------
function(psd_append_file_abs out_file abs_path rel_label)
  if(EXISTS "${abs_path}")
    psd_file_header("${out_file}" "${rel_label}")
    file(READ "${abs_path}" _content)
    file(APPEND "${out_file}" "${_content}\n\n")
  endif()
endfunction()

# --------------------------------------------------------------------------------
# File patterns to include
# --------------------------------------------------------------------------------
set(PATTERNS "")
foreach(ext IN LISTS INCLUDE_EXTENSIONS)
  string(REGEX REPLACE "^\\." "" _ext "${ext}")
  list(APPEND PATTERNS "*.${_ext}")
endforeach()

# --------------------------------------------------------------------------------
# Collect candidate files
# --------------------------------------------------------------------------------
set(ALL_FILES "")
foreach(p IN LISTS PATTERNS)
  file(GLOB_RECURSE TMP
       RELATIVE "${PROJECT_ROOT}"
       "${PROJECT_ROOT}/${p}")
  list(APPEND ALL_FILES ${TMP})
endforeach()

# --------------------------------------------------------------------------------
# HARD EXCLUSIONS
# --------------------------------------------------------------------------------
foreach(pattern IN LISTS EXCLUDE_DIRS)
  if(pattern MATCHES "\\*$")
    string(REPLACE "*" "" prefix "${pattern}")
    string(REPLACE "." "\\\\." prefix_esc "${prefix}")
    string(REPLACE "-" "\\\\-" prefix_esc "${prefix_esc}")
    list(FILTER ALL_FILES EXCLUDE REGEX "(^|/)${prefix_esc}[^/]*/")
  else()
    set(exact "${pattern}")
    string(REPLACE "." "\\\\." exact_esc "${exact}")
    string(REPLACE "-" "\\\\-" exact_esc "${exact_esc}")
    list(FILTER ALL_FILES EXCLUDE REGEX "(^|/)${exact_esc}/")
  endif()
endforeach()

list(REMOVE_ITEM ALL_FILES "Project-Source.txt")
list(REMOVE_DUPLICATES ALL_FILES)
list(SORT ALL_FILES)
list(LENGTH ALL_FILES FILE_COUNT)

file(APPEND "${PROJECT_ROOT}/ProjectSource-Debug.log"
  "ALL_FILES count=${FILE_COUNT}\n"
)

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
# Project tree
# --------------------------------------------------------------------------------
psd_write_project_tree("${OUT_FILE}" "${PROJECT_ROOT}")

# --------------------------------------------------------------------------------
# Always include CMakeLists.txt explicitly
# --------------------------------------------------------------------------------
psd_append_file_abs("${OUT_FILE}"
                    "${PROJECT_ROOT}/CMakeLists.txt"
                    "CMakeLists.txt")

# --------------------------------------------------------------------------------
# Write file contents
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
