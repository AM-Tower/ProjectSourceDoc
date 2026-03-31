#!/usr/bin/env bash
#****************************************************************
# @file backup.sh
# @brief Timestamped backup utility for Qwen3-TTS.
#        Copies only selected source tree structure and file types
#        while excluding heavy folders.
# @author Jeffrey Scott Flesher with the help of AI: Copilot
# @date Version: SEE BELOW CODE SCRIPT_VERSION_BACKUP
# @version Version: SEE BELOW CODE SCRIPT_DATE_BACKUP
#****************************************************************
declare -gx SCRIPT_VERSION_BACKUP="0.0.2";
declare -gx SCRIPT_DATE_BACKUP="[2026-03-15 @ 0800]";

#****************************************************************
# @section Global Exclude folders for each SWITCH_PROJECT:
#   --project <Qwen3-TTS SadTalker-Fork>
# .cache: repo files without patches, used to update repos
# .venv: --project venv
# .git: local git repo
# Archive: Files and Folders I save but do not git
# assets: Repo assets like models or checkpoints
# Out: Output Folders for all projects
# repos: repo from .cache maybe with patches
#****************************************************************

#****************************************************************
# @brief Create a timestamped backup of the project.
#        Only file types listed in INCLUDE_EXTS are included.
#        Heavy folders are excluded using EXCLUDE_FOLDERS.
#        Keeps folder tree structure for included folders.
# @return None.
#****************************************************************
run_backup()
{
    show_header "${BLUE}";
    if [[ -z "${PATH_BACKUP}" ]]; then log_msg "Error" "PATH_BACKUP is empty --backup"; return 1; fi
    if [[ -z "${PATH_ROOT}" ]]; then log_msg "Error" "PATH_ROOT is not set"; return 1; fi
    if [[ ! -d "${PATH_ROOT}" ]]; then log_msg "Error" "PATH_ROOT does not exist: ${PATH_ROOT}"; return 1; fi
    local base_path="${PATH_BACKUP}";
    local ts; ts="$(date +"%Y-%m-%d_%H%M")"
    local source_dir="${PATH_ROOT}";
    local dest="${base_path}/${ts}";
    log_msg "Info" "Creating backup at: ${dest}";
    if [[ ! -d "${dest}" ]]; then if ! mkdir -p "${dest}"; then log_msg "Error" "Creating backup at: ${dest}"; return 1; fi; fi
    local exclude_args;
    exclude_args=();
    local folder="";
    for folder in "${EXCLUDE_FOLDERS[@]}"; do
        exclude_args+=("--exclude=${folder}/");
    done
    local include_args;
    include_args=();
    include_args+=("--include=/main.sh");
    include_args+=("--include=*/");
    for folder in "${INCLUDE_FOLDERS[@]}"; do
        include_args+=("--include=/${folder}/**");
    done
    local ext="";
    for ext in "${INCLUDE_EXTS[@]}"; do
        if [[ "${ext}" == .* ]]; then
            include_args+=("--include=${ext}");
        else
            include_args+=("--include=*.${ext}");
        fi
    done
    rsync -av \
        --prune-empty-dirs \
        "${exclude_args[@]}" \
        "${include_args[@]}" \
        --exclude='*' \
        "${source_dir}/" "${dest}/";
    log_msg "Pass" "Backup completed at ${dest}";
    show_footer "${BLUE}";
    return 0; # Pass
}

#****************************************************************

log_msg "Info" "Loaded: backup.sh version (${SCRIPT_VERSION_BACKUP}) ${SCRIPT_DATE_BACKUP}"

#****************************************************************
# @footer End of backup.sh
#****************************************************************
