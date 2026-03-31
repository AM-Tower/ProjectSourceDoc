# --------------------------------------------------------------------------------------------------
# CMake
# @file CMakeLists.txt
# Project: ProjectSourceDoc
# @brief Project CMake file
# @authors Jeffrey Scott Flesher with the help of AI: Copilot
# @date 2026-03-30
# --------------------------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.25)

project(ProjectSourceDoc
    VERSION 0.1.0
    LANGUAGES CXX
)

# --------------------------------------------------------------------------------------------------
# Disable TIFF support (avoids libtiff.so.5 dependency on Fedora)
# --------------------------------------------------------------------------------------------------
set(QT_IMAGEFORMATS_DISABLE_TIFF ON)

# --------------------------------------------------------------------------------------------------
# cmake/ folder: add to module search path
# --------------------------------------------------------------------------------------------------
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# --------------------------------------------------------------------------------------------------
# C++ standard
# --------------------------------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --------------------------------------------------------------------------------------------------
# Qt automation
# --------------------------------------------------------------------------------------------------
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# --------------------------------------------------------------------------------------------------
# Qt dependencies
# --------------------------------------------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Qt6 OPTIONAL_COMPONENTS Svg Concurrent)

qt_standard_project_setup()

# --------------------------------------------------------------------------------------------------
# Sources / Headers
# --------------------------------------------------------------------------------------------------
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindowTheme.cpp
    src/StatusBarQueue.cpp
    src/AppLogUtils.cpp
    src/ProjectPaths.cpp
    src/EmbeddedDocs.cpp
    src/ProjectSourceExporter.cpp
)

set(HEADERS
    src/MainWindow.h
    src/StatusBarQueue.h
    src/AppLogUtils.h
    src/ProjectPaths.h
    src/EmbeddedDocs.h
    src/ProjectSourceExporter.h
)

set(DOC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/README.md
)

set(APP_QRC
    resources/ProjectSourceDoc.qrc
)

# --------------------------------------------------------------------------------------------------
# Main application target
# --------------------------------------------------------------------------------------------------
qt_add_executable(ProjectSourceDoc
    WIN32
    MACOSX_BUNDLE
    ${SOURCES}
    ${HEADERS}
    ${DOC_FILES}
    ${APP_QRC}
)

# --------------------------------------------------------------------------------------------------
# Qt plugin selection
# Exclude TIFF image plugin to avoid libtiff dependency
# --------------------------------------------------------------------------------------------------
qt_import_plugins(ProjectSourceDoc
    EXCLUDE Qt::QTiffPlugin
)

# --------------------------------------------------------------------------------------------------
# Include paths
# --------------------------------------------------------------------------------------------------
target_include_directories(ProjectSourceDoc PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# --------------------------------------------------------------------------------------------------
# Link libraries
# --------------------------------------------------------------------------------------------------
target_link_libraries(ProjectSourceDoc PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

if(TARGET Qt6::Svg)
    target_link_libraries(ProjectSourceDoc PRIVATE Qt6::Svg)
endif()

if(TARGET Qt6::Concurrent)
    target_link_libraries(ProjectSourceDoc PRIVATE Qt6::Concurrent)
endif()

# --------------------------------------------------------------------------------------------------
# cmake scripts (IDE visibility only)
# --------------------------------------------------------------------------------------------------
file(GLOB_RECURSE PROJECT_CMAKE_SCRIPTS
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/*.cmake"
)

if(PROJECT_CMAKE_SCRIPTS)
    add_custom_target(cmake_scripts SOURCES ${PROJECT_CMAKE_SCRIPTS})
endif()

# --------------------------------------------------------------------------------------------------
# Install + Packaging
# NOTE:
#  - The application CREATES its own .desktop file at runtime.
#  - Therefore, NO .desktop file is installed by CMake.
#  - deploy.sh / AppImage handles desktop integration.
# --------------------------------------------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS ProjectSourceDoc
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install CMake helper files (used by the app and tooling)
install(DIRECTORY cmake/
    DESTINATION share/${PROJECT_NAME}/cmake
)

# Install the SVG icon (used by AppImage + desktop integration)
install(FILES resources/icons/light/app.svg
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps
    RENAME ProjectSourceDoc.svg
)

# --------------------------------------------------------------------------------------------------
# CPack (optional, not used by AppImage but kept for future packaging)
# --------------------------------------------------------------------------------------------------
include(InstallRequiredSystemLibraries)

set(CPACK_PACKAGE_NAME "ProjectSourceDoc")
set(CPACK_PACKAGE_VENDOR "ProjectSourceDoc")
set(CPACK_PACKAGE_CONTACT "support@ProjectSourceDoc.app")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "ProjectSourceDoc – Project source aggregation tool")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/packages")

if(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "ProjectSourceDoc")
endif()

include(CPack)

# --------------------------------------------------------------------------------------------------
# AppImage Packaging (Linux only)
# --------------------------------------------------------------------------------------------------
if(UNIX AND NOT APPLE)
    message(STATUS "AppImage packaging is handled externally by deploy.sh")
endif()

# --------------------------------------------------------------------------------------------------
# End of CMakeLists.txt
# --------------------------------------------------------------------------------------------------
