# RELEASE.md
========================================================================================================================

This document defines the **official release workflow**, **command intent**, and **formatting rules** for this repository.

The goal is to ensure that:

- GitHub maintenance is safe and routine
- Releases are tested before publishing
- Publishing is always explicit and intentional
- `VERSION`, AppImage artifacts, and the updater stay in sync
- Accidental releases are impossible

This workflow is **manual by design** and does **not** rely on CI.

------------------------------------------------------------------------------------------------------------------------
## Core Principles
------------------------------------------------------------------------------------------------------------------------

1. **GitHub updates are not releases**
2. **Releases must be tested before publishing**
3. **Publishing requires explicit intent**
4. **VERSION is the single source of truth**
5. **deploy.sh is the release gate**

Nothing outside `deploy.sh` should mutate release state.

------------------------------------------------------------------------------------------------------------------------
## Terminology
------------------------------------------------------------------------------------------------------------------------

- **GitHub Maintenance**
  - Documentation edits
  - Script fixes
  - File renames / deletions
  - No change to `VERSION`

- **Release Finalization**
  - Incrementing `VERSION`
  - Building AppImage
  - Running runtime tests
  - No publishing

- **Release Publishing**
  - Committing `VERSION`
  - Pushing to GitHub
  - Making the updater see the new version

These are **three separate activities**.

------------------------------------------------------------------------------------------------------------------------
## Command Matrix (Authoritative)
------------------------------------------------------------------------------------------------------------------------

| Intent                  | Command                         | VERSION | GitHub Push | AppImage |
|------------------------|----------------------------------|---------|-------------|----------|
| GitHub maintenance     | `./deploy.sh --github`           | No      | Yes         | No       |
| Release testing        | `./deploy.sh --final`            | Yes     | No          | Yes      |
| Release publishing     | `./deploy.sh --final --github`   | Yes     | Yes         | Yes      |

**No other combinations are valid.**

------------------------------------------------------------------------------------------------------------------------
## Phase 1: GitHub Maintenance (No Release)
------------------------------------------------------------------------------------------------------------------------

### Purpose
Used for:
- README updates
- Documentation changes
- Script cleanup
- Renaming or deleting files
- Non‑release fixes

### Command
```bash
./deploy.sh --github


Expected Behavior

Local commit is created
Changes are pushed to GitHub
VERSION remains unchanged
No AppImage version bump
No release semantics are triggered

Rules

Do NOT modify VERSION
Do NOT assume a release is happening
This is safe to run frequently


Phase 2: Release Finalization (Local Only)

Purpose
Used to:

Prepare a new release
Increment VERSION
Build and test the AppImage
Verify runtime behavior
Validate updater logic

Command
./deploy.sh --final
Expected Behavior

VERSION is incremented locally
AppImage is built with the new version
Full test suite runs
No GitHub push occurs

Rules

This step MUST pass before publishing
You may repeat this step multiple times
GitHub must remain unchanged


Phase 3: Release Publishing (Explicit)

Purpose
Used to:

Publish a tested release
Sync GitHub with the release state
Make the updater detect the new version

Command
./deploy.sh --final --github

Expected Behavior

VERSION is committed
GitHub is updated
AppImage version matches GitHub VERSION
Updater reports the new version

Rules

Only run this after successful local testing
This is the ONLY command that publishes a release
Publishing without testing is forbidden


VERSION Rules


VERSION is the single source of truth
Format: MAJOR.MINOR.PATCH
VERSION MUST NOT be edited manually during normal work
VERSION MUST only change during --final
The AppImage filename MUST match VERSION
GitHub /VERSION MUST match the published AppImage


Formatting & Repository Rules

Files

Files without extensions (e.g. VERSION, LICENSE) are valid and tracked
.gitignore is the only exclusion mechanism
Do not rely on filename extensions for logic

Documentation

Keep README.md user‑facing
Keep RELEASE.md process‑facing
Update documentation freely during GitHub maintenance

Commits

GitHub maintenance commits may be frequent
Release commits should be minimal and intentional
Avoid mixing documentation fixes with release commits


Safety Guarantees

This workflow guarantees:

No accidental releases
No accidental GitHub pushes during testing
No version drift between binary and updater
Reproducible release artifacts
Clear intent behind every command

If something went wrong, it is always traceable by:

VERSION
Git history
deploy logs


Non‑Goals

This project intentionally does NOT:

Auto‑release from CI
Auto‑publish on commit
Infer release intent
Hide release state

Everything is explicit.

Summary


GitHub updates ≠ releases
Testing ≠ publishing
Publishing requires --final --github
VERSION is authoritative
deploy.sh enforces intent

If you are unsure which command to run, do not publish.

End of File