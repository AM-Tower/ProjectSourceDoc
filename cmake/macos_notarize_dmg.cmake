# --------------------------------------------------------------------------------------------------
# CMake
# @file cmake/macos_notarize_dmg.cmake
# @brief Notarize DMG macOS packages
# @authors Jeffrey Scott Flesher with the help of AI: Copilot
# @date 2026-03-31
# @details
# Inputs:
#   DMG_DIR        - directory where the DMG was generated
#   NOTARY_PROFILE - credential profile name created via `notarytool store-credentials`
# --------------------------------------------------------------------------------------------------

if(NOT DEFINED DMG_DIR OR DMG_DIR STREQUAL "")
  message(FATAL_ERROR "DMG_DIR not set")
endif()

# If no notary profile provided, skip notarization cleanly.
if(NOT DEFINED NOTARY_PROFILE OR NOTARY_PROFILE STREQUAL "")
  message(STATUS "NOTARY_PROFILE not set; skipping notarization/stapling.")
  return()
endif()

# Find the newest DMG in the directory.
file(GLOB DMGS "${DMG_DIR}/*.dmg")
if(NOT DMGS)
  message(FATAL_ERROR "No DMG found in ${DMG_DIR}")
endif()

# Pick the last one after sorting by name (common when versioned),
# or you can customize this to match CPACK_PACKAGE_FILE_NAME if you prefer.
list(SORT DMGS)
list(GET DMGS -1 DMG_PATH)

message(STATUS "Notarizing DMG: ${DMG_PATH}")

# notarytool supports authentication profiles and submit --wait. [4](https://keith.github.io/xcode-man-pages/notarytool.1.html)
execute_process(
  COMMAND xcrun notarytool submit "${DMG_PATH}" --wait -p "${NOTARY_PROFILE}"
  RESULT_VARIABLE NOTARY_RESULT
)

if(NOTARY_RESULT)
  message(FATAL_ERROR "notarytool submit failed with code: ${NOTARY_RESULT}")
endif()

# Apple describes stapling the ticket to the software after notarization. [3](https://developer.apple.com/documentation/security/notarizing-macos-software-before-distribution)
# notarytool docs reference stapling ("see stapler(1)"). [4](https://keith.github.io/xcode-man-pages/notarytool.1.html)
execute_process(
  COMMAND xcrun stapler staple "${DMG_PATH}"
  RESULT_VARIABLE STAPLE_RESULT
)

if(STAPLE_RESULT)
  message(FATAL_ERROR "stapler staple failed with code: ${STAPLE_RESULT}")
endif()

message(STATUS "Notarization + stapling completed: ${DMG_PATH}")

# --------------------------------------------------------------------------------------------------
# End of cmake/macos_notarize_dmg.cmake
# --------------------------------------------------------------------------------------------------
