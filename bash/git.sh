#!/usr/bin/env bash
#****************************************************************
# @file git.sh
# @brief Qwen3-TTS git functions.
#
# @author Jeffrey Scott Flesher with the help of AI: Copilot
# @date Version: SEE BELOW CODE SCRIPT_VERSION_GIT
# @version Version: SEE BELOW CODE SCRIPT_DATE_GIT
#****************************************************************
declare -gx SCRIPT_VERSION_GIT="0.0.5";
declare -gx SCRIPT_DATE_GIT="[2026-03-30 @ 0800]";

#****************************************************************
# @brief git init function.
# @return None.
#****************************************************************
git_init()
{
    show_header "${SKYBLUE}";
    log_msg "Warning" "Running: git init: ${PATH_ROOT}";
    if [ -d "${PATH_ROOT}/.git" ]; then
        log_msg "Warning" "Removing .git folder ";
        rm -rf "${PATH_ROOT}/.git";
    fi
    if ! cd "${PATH_ROOT}"; then log_msg "Error" "Failed to cd to ${PATH_ROOT}"; fi
    if ! git init; then log_msg "Error" "Failed git init"; return 1; fi
    git config --global core.trustctime false;
    git config advice.addIgnoredFile false;
    git branch -m main;
    git config --global init.defaultBranch main;
    git add .;
    if ! git commit -m "Initial commit after reinitializing repository"; then log_msg "Error" "Failed to git commit"; fi
    show_footer "${SKYBLUE}";
    return 0; # Pass
}
export -f git_init;

#****************************************************************
# @brief Create a local git repo in the current folder if missing,
#        and commit only allowed file types in root + selected folders.
# @return None.
#****************************************************************
git_check()
{
    #*********************
    # return here
    #*********************
    return_here()
    {
        local -i rc="${1:-0}";
        show_footer "${SKYBLUE}";
        return "${rc}";
    }
    # Start with a Header
    show_header "${SKYBLUE}";
    local dt; dt="$(date +"%Y-%d-%b %A %H:%M:%S")";
    log_msg "Info" "Local git auto check in at ${dt}";
    # Check .git folder
    if [[ ! -d ".git" ]]; then
        log_msg "Info" "No .git folder found. Initializing a local git repository...";
        if [ -d "${PATH_ROOT}" ]; then
            if [ "$(pwd)" != "${PATH_ROOT}" ]; then
                if ! cd "${PATH_ROOT}"; then
                    log_msg "Error" "Failed to cd: $(PATH_ROOT)";
                    return 1;
                fi
            fi
        else
            log_msg "Error" "Failed to find: $(PATH_ROOT)";
            return 1;
        fi
        # git init
        if git init; then
            log_msg "Pass" "Local git repository initialized in: $(pwd)";
        else
            log_msg "Error" "Failed to initialize Local git repository in: $(pwd)";
        fi
    else
        log_msg "Info" "Git repository already exists: $(pwd)";
    fi

    #****************************************************************
    # @brief Check if a path is ignored by gitignore.
    # @param path File path.
    # @return 0 if ignored, 1 otherwise.
    #****************************************************************
    is_git_ignored()
    {
        if ! git check-ignore -q -- "${1}"; then return 1; fi
        return 0; # Pass
    }

    #****************************************************************
    # @brief Check if file extension is allowed.
    # @param path File path.
    # @return 0 if allowed, 1 otherwise.
    #****************************************************************
    is_allowed_ext()
    {
        local path="${1}";
        [[ "${path}" == *.* ]] || return 1;
        local ext="${path##*.}"; ext="${ext,,}";
        local e;
        for e in "${INCLUDE_EXTS[@]}"; do
            if [[ "${ext}" == "${e}" ]]; then return 0; fi
        done
        return 1; # Fail
    }

    #****************************************************************
    # @brief Detect if path is inside a git submodule.
    # @param path File path.
    # @return 0 if inside submodule, 1 otherwise.
    #****************************************************************
    is_in_submodule()
    {
        local path="${1}";
        if [[ -z "${path}" ]]; then return 1; fi
        local dir; dir="$(dirname -- "${path}")";
        while [[ "${dir}" != "." && "${dir}" != "/" ]]; do
            if [[ -e "${dir}/.git" ]]; then return 0; fi
            dir="$(dirname -- "${dir}")";
        done
        return 1; # Fail
    }

    #****************************************************************
    # @brief Stage root files + included folders, skipping excluded folders.
    #****************************************************************
    log_msg "Info" "Staging root files + \${INCLUDE_FOLDERS[*]} (excluding \${EXCLUDE_FOLDERS[*]})..."

    if [[ -f ".gitignore" ]]; then
        git add ".gitignore"
        log_msg "Pass" "Staged .gitignore"
    fi

    shopt -s dotglob nullglob
    local file
    for file in *; do
        [[ -f "${file}" ]] || continue
        local -i skip
        skip=0
        local ex
        for ex in "${EXCLUDE_FOLDERS[@]}"; do
            if [[ "${file}" == "${ex}" ]]; then
                skip=1
                break
            fi
        done
        if ((skip == 1)); then continue; fi
        # is allowed ext
        if ! is_allowed_ext "${file}"; then
            log_msg "Info" "Skipped (ext not allowed): ${file}"
            continue
        fi
        if is_git_ignored "${file}"; then
            log_msg "Info" "Ignored by .gitignore (root): ${file}"
            continue
        fi
        git add -- "${file}"
    done
    shopt -u dotglob nullglob

    local prune_expr
    prune_expr=()
    local ex2
    for ex2 in "${EXCLUDE_FOLDERS[@]}"; do
        prune_expr+=(-name "${ex2}" -o)
    done
    unset 'prune_expr[-1]'

    local inc
    for inc in "${INCLUDE_FOLDERS[@]}"; do
        if [[ ! -d "${inc}" ]]; then continue; fi
        local f
        while IFS= read -r -d '' f; do
            if is_in_submodule "${f}"; then
                log_msg "Info" "Skipped (in submodule): ${f}"
                continue
            fi
            if ! is_allowed_ext "${f}"; then continue; fi
            if is_git_ignored "${f}"; then
                log_msg "Info" "Ignored by .gitignore: ${f}"
                continue
            fi
            git add -- "${f}"
        done < <(find "${inc}" \( -type d \( "${prune_expr[@]}" \) -prune \) -o -type f -print0)
    done

    if git diff --cached --quiet; then
        log_msg "Info" "No changes to commit."
    else
        local commit_msg
        commit_msg="Local wildcard commit: $(date '+%Y-%m-%d %H:%M:%S')"
        git commit -m "${commit_msg}"
        log_msg "Pass" "Committed: ${commit_msg}"
    fi

    return_here 0; # Return code
}
export -f git_check;

#****************************************************************

log_msg "Info" "Loaded: git.sh version (${SCRIPT_VERSION_GIT}) ${SCRIPT_DATE_GIT}"

#****************************************************************
# @footer End of git.sh
#****************************************************************
