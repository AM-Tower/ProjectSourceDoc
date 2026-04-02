# --------------------------------------------------------------------------------------------------
# CMake
# @file    CMakeLists.txt
# Project: ProjectSourceDoc
# @brief   Project CMake file
# @authors Jeffrey Scott Flesher with the help of AI: Copilot
# @date    2026-04-02
# --------------------------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.25)

project(ProjectSourceDoc VERSION 0.1.0 LANGUAGES CXX)

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
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets LinguistTools)
find_package(Qt6 OPTIONAL_COMPONENTS Svg Concurrent)

qt_standard_project_setup()

# --------------------------------------------------------------------------------------------------
# Sources / Headers
# --------------------------------------------------------------------------------------------------
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindowTheme.cpp
    src/AppLogUtils.cpp
    src/EmbeddedDocs.cpp
    src/StatusBarQueue.cpp
    src/ProjectPaths.cpp
    src/ProjectSourceExporter.cpp
    src/CMakeSourceParser.cpp
    src/GitIgnoreMatcher.cpp
)

set(HEADERS
    src/MainWindow.h
    src/AppLogUtils.h
    src/EmbeddedDocs.h
    src/StatusBarQueue.h
    src/ProjectPaths.h
    src/ProjectSourceExporter.h
    src/CMakeSourceParser.h
    src/GitIgnoreMatcher.h
    src/ProjectNode.h
)

set(DOC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/README.md
)

set(APP_QRC
    resources/ProjectSourceDoc.qrc
)

# --------------------------------------------------------------------------------------------------
# Translations (multi-language)
# --------------------------------------------------------------------------------------------------
file(GLOB APP_TRANSLATIONS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/i18n/ProjectSourceDoc_*.ts"
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
# Generate and embed translations (Qt 6 – resource-based, correct usage)
# --------------------------------------------------------------------------------------------------
if(APP_TRANSLATIONS)
    qt6_add_translations(ProjectSourceDoc
        TS_FILES ${APP_TRANSLATIONS}
        RESOURCE_PREFIX "/i18n"
    )
else()
    message(STATUS "No translation files found (i18n/ProjectSourceDoc_*.ts)")
endif()

# --------------------------------------------------------------------------------------------------
# Qt plugin selection
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
# Unit Tests
# --------------------------------------------------------------------------------------------------
option(BUILD_TESTS "Build ProjectSourceDoc unit tests" ON)

if(BUILD_TESTS)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Test)

    add_executable(ProjectSourceDocTests
        tests/ProjectSourceDocTests.cpp
        src/MainWindow.cpp
        src/MainWindowTheme.cpp
        src/AppLogUtils.cpp
        src/EmbeddedDocs.cpp
        src/StatusBarQueue.cpp
        src/ProjectPaths.cpp
        src/ProjectSourceExporter.cpp
        src/CMakeSourceParser.cpp
        src/GitIgnoreMatcher.cpp
        resources/ProjectSourceDoc.qrc
    )

    target_include_directories(ProjectSourceDocTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )

    target_link_libraries(ProjectSourceDocTests PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Test
    )

    add_test(NAME ProjectSourceDocTests COMMAND ProjectSourceDocTests)
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
# --------------------------------------------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS ProjectSourceDoc
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY cmake/
    DESTINATION share/${PROJECT_NAME}/cmake
)

install(FILES resources/icons/light/app.svg
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps
    RENAME ProjectSourceDoc.svg
)

# --------------------------------------------------------------------------------------------------
# CPack
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
# AppImage Packaging
# --------------------------------------------------------------------------------------------------
if(UNIX AND NOT APPLE)
    message(STATUS "AppImage packaging is handled externally by deploy.sh")
endif()

# --------------------------------------------------------------------------------------------------
# End of cmake
# --------------------------------------------------------------------------------------------------
