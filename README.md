# ProjectSourceDoc

ProjectSourceDoc is a Qt-based desktop application that generates a single,
fully packaged Project-Source.txt file from a CMake project.

The output is designed specifically for AI review, auditing, backups,
and long-term archival, with deterministic filtering and a clearly
structured project tree.

---

## Features

- Generate a single Project-Source.txt using CMake script mode
- Deterministic project tree (directories first, files last)
- Per-project include and exclude rules
- Gitignore integration for excluded directories
- Timestamped project backups
- Cross-platform Qt Widgets UI
- Clean separation between UI, exporter logic, and CMake logic

---

## Generated Outputs

### Project-Source.txt

A single, self-contained source package that includes:

- Header metadata (project root, includes, excludes, file count)
- Project directory tree
- Explicit inclusion of CMakeLists.txt
- Full contents of all included source files

This file is intended for:

- AI code review
- Architecture analysis
- Offline inspection
- Long-term archival

### Project-Tree.txt

A standalone tree listing of the project structure, generated on demand.

### Backups

Each run can create a timestamped backup at:

    <backup-base>/<project-name>/YYYY-MM-DD_HH-MM-SS/

---

## Directory Structure

    ProjectSourceDoc/
    ├── cmake/
    │   └── PackSource.cmake
    ├── src/
    │   ├── MainWindow.cpp
    │   ├── MainWindow.h
    │   ├── ProjectSourceExporter.cpp
    │   ├── ProjectSourceExporter.h
    │   └── ...
    ├── resources/
    ├── i18n/
    ├── bash/
    ├── CMakeLists.txt
    └── README.md

---

## Architecture Overview

### UI Layer (Qt)

- MainWindow
  - Manages project selection
  - Stores per-project settings using QSettings
  - Invokes backup and source generation
  - Displays logs and status

### Export Layer (C++)

- ProjectSourceExporter
  - Creates timestamped backups
  - Invokes CMake in script mode
  - Captures logs and errors
  - Returns generated output paths to the UI

### Packaging Layer (CMake)

- cmake/PackSource.cmake
  - Receives parameters via -D
  - Builds the project tree using tree (if available)
  - Applies robust include and exclude rules
  - Writes Project-Source.txt

---

## CMake Script Invocation

The exporter invokes CMake like this:

    cmake \
      -DPROJECT_ROOT=/absolute/path/to/project \
      -DINCLUDE_EXTENSIONS=cpp;h;md \
      -DEXCLUDE_DIRS=build*;.git;.qtcreator \
      -P cmake/PackSource.cmake

Important:
When using CMake script mode (-P), all -D variables must appear
before -P.

---

## Exclusion Rules

Excluded directories may come from:

1. Default rules
2. User-defined settings
3. Gitignore (directory rules only)

Examples:

    build*
    cmake-build-*
    deploy*
    .git
    0-Archive

All exclusions are applied consistently to:

- Tree output
- File inclusion
- Backups

---

## Dependencies

### Required

- Qt 6 (Widgets)
- CMake 3.25 or newer

### Optional

- tree (used for enhanced directory tree output)

If tree is not installed, generation still succeeds with a fallback message.

---

## Build Instructions

    mkdir build
    cd build
    cmake ..
    cmake --build .

---
## Git

    git add .
    git commit -m "Updated features, and bugs."
    git push
    git push -u origin main

---

## License

Copyright 2026  
Jeffrey Scott Flesher

This project is intended for personal, professional,
and AI-assisted development workflows.

---

## Author

Jeffrey Scott Flesher  
Created with the assistance of Microsoft Copilot and iterative AI tooling.

---

End of README.md