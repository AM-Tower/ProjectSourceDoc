#!/usr/bin/env bash
#**************************************************************************************************
# @file         library.sh
# @brief        Bash Library functions.
# @author       Jeffrey Scott Flesher with the help of AI: Copilot
# @date         Version: SEE BELOW CODE SCRIPT_VERSION_LIBRARY
# @version      Version: SEE BELOW CODE SCRIPT_DATE_LIBRARY
#
#**************************************************************************************************
declare -gx SCRIPT_VERSION_LIBRARY="0.0.6";
declare -gx SCRIPT_DATE_LIBRARY="[2026-02-23 @ 12:00]";

declare -gxi UNIT_TEST_LIBRARY_SHOW_DEBUG=1;
declare -gxi UNIT_TEST_LIBRARY_SHOW_PASS=1;

#****************************************************************
# @brief Validate that a path is a valid Unix-style folder path.
# @param $1 path to validate.
# @return 0 if valid Unix path, 1 otherwise.
#****************************************************************
is_fold_unix()
{
    if (($# != 1)); then log_msg "Error" "is_fold_unix: requires exactly 1 argument"; return 1; fi
    # Check path
    local path="${1}";
    # Empty path is invalid
    if [[ -z "${path}" ]]; then return 1; fi
    # Reject Windows drive letters: C:\foo, D:/bar, etc.
    if [[ "${path}" =~ ^[A-Za-z]:[\\/].* ]]; then return 1; fi
    # Reject UNC paths: \\server\share
    if [[ "${path}" =~ ^\\\\ ]]; then return 1; fi
    # Reject any backslashes at all — Unix paths never contain them
    if [[ "${path}" == *"\\"* ]]; then return 1; fi
    # Reject CRLF contamination
    if printf "%s" "${path}" | grep -q $'\r'; then return 1; fi
    # At this point, the path is syntactically Unix-like.
    return 0; # Pass
}

#****************************************************************
# @brief Validate that a path is a valid Unix-style path.
# @param $1 path to validate.
# @return 0 if valid Unix path, 1 otherwise.
#****************************************************************
is_file_unix()
{
    if (($# != 1)); then log_msg "Error" "is_file_unix: requires exactly 1 argument"; return 1; fi
    # Check path
    local path="${1}";
    # Empty path is invalid
    if [[ -z "${path}" ]]; then return 1; fi
    # Reject Windows drive letters: C:\foo, D:/bar, etc.
    if [[ "${path}" =~ ^[A-Za-z]:[\\/].* ]]; then return 1; fi
    # Reject UNC paths: \\server\share
    if [[ "${path}" =~ ^\\\\ ]]; then return 1; fi
    # Reject any backslashes at all — Unix paths never contain them
    if [[ "${path}" == *"\\"* ]]; then return 1; fi
    # Reject paths containing CRLF artifacts
    if printf "%s" "${path}" | grep -q $'\r'; then return 1; fi
    # At this point, the path is syntactically Unix-like.
    return 0; # Pass
}

#****************************************************************
# @brief Unit Test for library.sh
# @return 0 if all tests pass, 1 otherwise.
#****************************************************************
unit_test_library()
{
    local -i fail=0;
    local old_virtual_env;
    #local out;
    show_header "${YELLOW}";
    log_msg "Info" "Running Unit Test for library.sh";
    # ------------------------------------------------------------
    _pass()
    {
        if ((UNIT_TEST_LIBRARY_SHOW_PASS == 1)); then log_msg "Pass" "${1}"; fi
    }
    _dbg()
    {
        if ((UNIT_TEST_LIBRARY_SHOW_DEBUG == 1)); then log_msg "Info" "${1}"; fi
    }
    _fail()
    {
        log_msg "Error" "${1}";
        fail=$((fail + 1));
    }
    # ------------------------------------------------------------
    show_footer "${YELLOW}";
    _dbg "Testing: is_fold_unix";
    if is_fold_unix > /dev/null 2>&1; then _fail "is_fold_unix should fail with 0 args"; else _pass "is_fold_unix rejects 0 args"; fi
    if is_fold_unix "a" "b" > /dev/null 2>&1; then _fail "is_fold_unix should fail with 2 args"; else _pass "is_fold_unix rejects 2 args"; fi
    if is_fold_unix "" > /dev/null 2>&1; then _fail "is_fold_unix should reject empty path"; else _pass "is_fold_unix rejects empty path"; fi
    if is_fold_unix "C:\\foo" > /dev/null 2>&1; then _fail "is_fold_unix should reject Windows drive path C:\\foo"; else _pass "is_fold_unix rejects Windows drive path"; fi
    if is_fold_unix "D:/bar" > /dev/null 2>&1; then _fail "is_fold_unix should reject Windows drive path D:/bar"; else _pass "is_fold_unix rejects Windows drive path with /"; fi
    if is_fold_unix "\\\\server\\share" > /dev/null 2>&1; then _fail "is_fold_unix should reject UNC path \\\\server\\share"; else _pass "is_fold_unix rejects UNC path"; fi
    if is_fold_unix "/tmp\\bad" > /dev/null 2>&1; then _fail "is_fold_unix should reject backslash in path"; else _pass "is_fold_unix rejects backslash in path"; fi
    if is_fold_unix $'/tmp/bad\r' > /dev/null 2>&1; then _fail "is_fold_unix should reject CR in path"; else _pass "is_fold_unix rejects CRLF contamination"; fi
    if is_fold_unix "/tmp" > /dev/null 2>&1; then _pass "is_fold_unix accepts /tmp"; else _fail "is_fold_unix should accept /tmp"; fi
    if is_fold_unix "relative/path" > /dev/null 2>&1; then _pass "is_fold_unix accepts relative/path"; else _fail "is_fold_unix should accept relative/path"; fi
    # ------------------------------------------------------------
    show_footer "${YELLOW}";
    _dbg "Testing: is_file_unix";
    if is_file_unix > /dev/null 2>&1; then _fail "is_file_unix should fail with 0 args"; else _pass "is_file_unix rejects 0 args"; fi
    if is_file_unix "a" "b" > /dev/null 2>&1; then _fail "is_file_unix should fail with 2 args"; else _pass "is_file_unix rejects 2 args"; fi
    if is_file_unix "" > /dev/null 2>&1; then _fail "is_file_unix should reject empty path"; else _pass "is_file_unix rejects empty path"; fi
    if is_file_unix "C:\\foo.txt" > /dev/null 2>&1; then _fail "is_file_unix should reject Windows drive path C:\\foo.txt"; else _pass "is_file_unix rejects Windows drive path"; fi
    if is_file_unix "\\\\server\\share\\x" > /dev/null 2>&1; then _fail "is_file_unix should reject UNC path"; else _pass "is_file_unix rejects UNC path"; fi
    if is_file_unix "/tmp\\bad.txt" > /dev/null 2>&1; then _fail "is_file_unix should reject backslash in path"; else _pass "is_file_unix rejects backslash in path"; fi
    if is_file_unix $'/tmp/bad.txt\r' > /dev/null 2>&1; then _fail "is_file_unix should reject CR in path"; else _pass "is_file_unix rejects CRLF contamination"; fi
    if is_file_unix "/tmp/file.txt" > /dev/null 2>&1; then _pass "is_file_unix accepts /tmp/file.txt"; else _fail "is_file_unix should accept /tmp/file.txt"; fi
    if is_file_unix "relative/file.py" > /dev/null 2>&1; then _pass "is_file_unix accepts relative/file.py"; else _fail "is_file_unix should accept relative/file.py"; fi
    # ------------------------------------------------------------
    show_footer "${YELLOW}";
    # Test is_venv_active
    _dbg "Testing: is_venv_active";
    #
    old_virtual_env="${VIRTUAL_ENV:-__UNSET__}";
    # Check if activated
    if [[ -n "${VIRTUAL_ENV:-}" ]]; then
        if declare -F deactivate >/dev/null 2>&1; then
            deactivate >/dev/null 2>&1 || true;
        elif command -v deactivate >/dev/null 2>&1; then
            # shellcheck source=/dev/null
            source deactivate >/dev/null 2>&1 || true;
        fi
    fi
    # Unset VIRTUAL_ENV
    unset VIRTUAL_ENV || true;
    if is_venv_active; then _fail "is_venv_active should be false when VIRTUAL_ENV is unset"; else _pass "is_venv_active is false when VIRTUAL_ENV is unset"; fi
    VIRTUAL_ENV="/tmp/fake-venv";
    if is_venv_active; then _pass "is_venv_active is true when VIRTUAL_ENV is set"; else _fail "is_venv_active should be true when VIRTUAL_ENV is set"; fi
    if [[ "${old_virtual_env}" == "__UNSET__" ]]; then unset VIRTUAL_ENV || true; else VIRTUAL_ENV="${old_virtual_env}"; fi
    # ------------------------------------------------------------
    if ((fail == 0)); then
        log_msg "Pass" "Passed Unit Test library.sh";
        show_footer "${YELLOW}";
        return 0;
    else
        log_msg "Error" "Failed Unit Test library.sh";
        show_footer "${YELLOW}";
        return 1;
    fi
    # Return code
}

#**************************************************************************************************
#**************************************************************************************************
# ADD to Test:
# Need test.mp4
#**************************************************************************************************
#**************************************************************************************************

#**************************************************************************************************
# @brief      Check whether a file is a valid, decodable MP4 video.
#             Uses ffprobe to validate container/metadata and ffmpeg to
#             detect decode errors (corruption, missing atoms, bad frames).
# @param      $1 Path to file (absolute).
# @return     0 if valid MP4 video, 1 otherwise.
#**************************************************************************************************
is_mp4()
{
    local file="${1:-}";
    if [[ -z "${file}" ]];    then log_msg "Error" "[${FUNCNAME[0]}] Missing file argument"; return 1; fi
    if [[ "${file}" != /* ]]; then log_msg "Error" "[${FUNCNAME[0]}] File path must be absolute: ${file}"; return 1; fi
    if [[ ! -f "${file}" ]];  then log_msg "Error" "[${FUNCNAME[0]}] File not found: ${file}"; return 1; fi
    if [[ ! -r "${file}" ]];  then log_msg "Error" "[${FUNCNAME[0]}] File not readable: ${file}"; return 1; fi
    case "${file,,}" in *.mp4) : ;; *)
        log_msg "Error" "[${FUNCNAME[0]}] Not an .mp4 file: ${file}"; return 1; ;;
    esac
    if ! command -v ffprobe > /dev/null 2>&1; then
        log_msg "Error" "[${FUNCNAME[0]}] ffprobe not found (ffmpeg package required)"; return 1;
    fi
    if ! command -v ffmpeg > /dev/null 2>&1; then
        log_msg "Error" "[${FUNCNAME[0]}] ffmpeg not found"; return 1;
    fi
    #-------------------------------------------------------------
    # Step 1: Container + metadata validation
    # ffprobe returns non-zero if file is not a valid media container
    #-------------------------------------------------------------
    if ! ffprobe -v error -show_format -show_streams -- "${file}" > /dev/null 2>&1; then
        log_msg "Error" "[${FUNCNAME[0]}] Invalid MP4 container or metadata: ${file}"; return 1;
    fi
    #-------------------------------------------------------------
    # Step 2: Decode validation
    # Reads all frames and reports decode errors without writing output
    #-------------------------------------------------------------
    if ffmpeg -v error -i "${file}" -f null - > /dev/null 2>&1; then
        log_msg "Pass" "[${FUNCNAME[0]}] Valid MP4 video: ${file}"; return 0;
    fi
    log_msg "Error" "[${FUNCNAME[0]}] MP4 decode errors detected (corrupt or incomplete): ${file}";
    return 1; # Fail
}

#****************************************************************
# @brief Prepare asset directories.
# @param
# REPO_ROOT_TTS_MAN="${REPO_ROOT}/${NAME_FOLDER_TTS_MAN}";
# REPO_ROOT="${PATH_ROOT}/repos"; # rm -rf ~/Qwen3-TTS/repos
#****************************************************************
folder_required()
{
    local root_folder="${1}";
    # Check if Folder exist or create it
    if [[ ! -d "${root_folder}" ]]; then
        if ! mkdir -p "${root_folder}"; then
            log_msg "Error" "Failed to make directory: ${root_folder}"; return 1;
        fi
    fi
    return 0; # Pass
}

#****************************************************************
# @brief  Check whether a URL is reachable.
# @param  $1 url
# @return 0 if reachable, non-zero otherwise
#****************************************************************
is_url()
{
    local url="${1:-}";
    [[ -z "${url}" ]] && return 1;
    # Prefer curl if available
    if command -v curl > /dev/null 2>&1; then
        curl --silent --head --fail --location --max-time 5 "${url}" > /dev/null; return;
    fi
    # Fallback to wget
    if command -v wget > /dev/null 2>&1; then
        wget --spider --quiet --timeout=5 "${url}"; return;
    fi
    # No HTTP client available
    return 1; # Fail
}

#****************************************************************
# @brief      Track elapsed time for named events across days.
# @param      event_name  Name of the event to track.
# @param      event_type  "Start" or "Stop". get_elapsed "${FUNCNAME[0]}" "Start";
# @return     Logs start/stop times to elapsed_times.txt and prints elapsed info.
#****************************************************************
get_elapsed()
{
    local event_name="${1:-}";
    local event_type="${2:-}";
    #
    if [ -z "${event_name}" ] || [ -z "${event_type}" ]; then
        log_msg "Error" "Invalid call to get_elapsed <event_name> <event_type>"; return 1;
    fi
    #
    local caller_script; caller_script="$(basename "${BASH_SOURCE[1]:-$0}")"
    local caller_func="${FUNCNAME[1]:-MAIN}";
    local caller_line="${BASH_LINENO[0]:-0}";
    local now; now=$(date +%s);
    local now_str; now_str="$(date '+%Y-%m-%d %H:%M:%S')";
    local et_file="elapsed_times.txt";
    touch "${et_file}";
    local start_line start_time stop_time last_et="unknown";
    # Internal helper to format elapsed seconds as dd:hh:mm:ss
    compute_et()
    {
        local seconds=$1;
        if [ "${seconds}" -lt 0 ]; then
            seconds=0;
        fi
        local d h m s
        printf -v d "%02d" $((seconds / 86400));
        printf -v h "%02d" $(((seconds % 86400) / 3600));
        printf -v m "%02d" $(((seconds % 3600) / 60));
        printf -v s "%02d" $((seconds % 60));
        printf "%s" "${d}:${h}:${m}:${s}";
    }
    #
    if [ "${event_type}" = "Start" ]; then
        # Check if this event already exists and calculate Last ET
        if grep -q "^${event_name}:Start:" "${et_file}" 2> /dev/null; then
            local stop_line; stop_line=$(grep "^${event_name}:Stop:" "${et_file}" 2> /dev/null | tail -n1 || true);
            local prev_start; prev_start=$(grep "^${event_name}:Start:" "${et_file}" 2> /dev/null | tail -n1 || true);
            # Only calculate if Stop line has a valid timestamp (not "Unknown")
            if [ -n "${stop_line}" ] && [ -n "${prev_start}" ] && [[ ! "${stop_line}" =~ Unknown ]]; then
                # Extract just the timestamp part after the last colon
                local prev_start_str="${prev_start##*:Start:}";
                local prev_stop_str="${stop_line##*:Stop:}";
                local prev_start_time; prev_start_time=$(date -d "${prev_start_str}" +%s 2> /dev/null || printf "%s" "0");
                local prev_stop_time; prev_stop_time=$(date -d "${prev_stop_str}" +%s 2> /dev/null || printf "%s" "0");
                #
                if [ "${prev_start_time}" -gt 0 ] && [ "${prev_stop_time}" -gt 0 ]; then
                    local et=$((prev_stop_time - prev_start_time));
                    last_et="$(compute_et "${et}")";
                    ELAPSED_TIMES_LAST["${event_name}"]="${last_et}";
                fi
            else
                last_et="${ELAPSED_TIMES_LAST[${event_name}]:-unknown}";
            fi
            # Remove old entries for this event
            grep -v "^${event_name}:" "${et_file}" > "${et_file}.tmp" || true;
            mv "${et_file}.tmp" "${et_file}";
        else
            last_et="${ELAPSED_TIMES_LAST[${event_name}]:-unknown}";
        fi
        # Add new Start and Stop:Unknown entries
        printf "%s" "${event_name}:Start:${now_str}" >> "${et_file}";
        printf "%s" "${event_name}:Stop:Unknown" >> "${et_file}";
        # Compute total elapsed since SCRIPT_START
        local total_elapsed=$((now - SCRIPT_START));
        local total_str; total_str="$(compute_et "${total_elapsed}")";
        log_msg "Info" "ET: ${event_name} Start | Total=${total_str} | Last ET: ${last_et} | ${caller_script}->${caller_func}->${caller_line}";
        # printf "%b  ET: %s Start | Total=%s | Last ET: %s | %s->%s->%s %b\n" "${GRAY}" "${event_name}" "${total_str}" "${last_et}" "${caller_script}" "${caller_func}" "${caller_line}" "${NC}";
    elif [ "${event_type}" = "Stop" ]; then
        # Find the Start line for this event
        start_line=$(grep "^${event_name}:Start:" "${et_file}" 2> /dev/null | tail -n1 || true);
        #
        if [ -n "${start_line}" ]; then
            # Extract just the timestamp part
            local start_str="${start_line##*:Start:}";
            start_time=$(date -d "${start_str}" +%s 2> /dev/null || printf "%s" "${now}");
            stop_time="${now}";
            local et=$((stop_time - start_time));
            last_et="$(compute_et "${et}")";
            ELAPSED_TIMES_LAST["${event_name}"]="${last_et}";
        else
            last_et="${ELAPSED_TIMES_LAST[${event_name}]:-unknown}";
        fi
        # Update Stop line with actual timestamp using awk
        if [ -f "${et_file}" ]; then
            awk -v event="${event_name}" -v timestamp="${now_str}" '$0 ~ "^" event ":Stop:" { print event ":Stop:" timestamp; next } { print }' "${et_file}" > "${et_file}.tmp";
            mv "${et_file}.tmp" "${et_file}";
        fi
        # Compute total elapsed since SCRIPT_START
        local total_elapsed=$((now - SCRIPT_START));
        local total_str; total_str="$(compute_et "${total_elapsed}")";
        log_msg "Info" "ET: ${event_name} Stop  | ET: ${last_et} | Total: ${total_str} | ${caller_script}->${caller_func}->${caller_line}";
        # printf "%b  ET: %s Stop  | ET: %s | Total: %s | %s->%s->%s %b\n" "${GRAY}" "${event_name}" "${last_et}" "${total_str}" "${caller_script}" "${caller_func}" "${caller_line}" "${NC}";
    fi

    return 0; # Pass
}

#**************************************************************************************************
# @brief Copy text to clipboard if supported.
# @param ${1} text Text to copy.
# @return 0 if copied, 1 if no clipboard tool, 2 if input empty.
#**************************************************************************************************
copy_to_clipboard()
{
    local text="${1:-}"
    if [[ -z "${text}" ]]; then return 2; fi
    # macOS
    if command -v pbcopy >/dev/null 2>&1; then printf "%s" "${text}" | pbcopy; return 0; fi
    # X11 (preferred on Linux desktops like yours)
    if command -v xclip >/dev/null 2>&1; then printf "%s" "${text}" | xclip -selection clipboard; return 0; fi
    # Wayland (fallback only)
    if command -v wl-copy >/dev/null 2>&1; then printf "%s" "${text}" | wl-copy; return 0; fi
    return 1; # false
}

#**************************************************************************************************
# @brief Print a command for user to paste, and try to copy it to clipboard.
# @param ${1} cmd Command string.
# @return 0 always.
#**************************************************************************************************
paste_cmd()
{
    local cmd="${1:-}";
    if [[ -z "${cmd}" ]]; then log_msg "Error" "paste_cmd called with empty cmd"; return 0; fi
    # printf "%s\n" "${cmd}";
    if copy_to_clipboard "${cmd}"; then log_msg "Info" "Copied to clipboard: ${cmd}"; else log_msg "Notice" "Clipboard tool not found; copy manually: ${cmd}"; fi
    return 0; # Pass
}

#**************************************************************************************************
# @brief Checks if conda is active (not supported).
# @param None.
# @return 0 if active, 1 if not.
#**************************************************************************************************
is_conda_active()
{
    if [[ -n "${CONDA_DEFAULT_ENV:-}" ]]; then return 0; fi
    if [[ -n "${CONDA_PREFIX:-}" ]]; then return 0; fi
    return 1; # Fail
}

#**************************************************************************************************
# @brief Checks if a Python virtual environment is active.
# @param None.
# @return 0 if active, 1 if not.
#**************************************************************************************************
is_venv_active()
{
    if [[ -n "${VIRTUAL_ENV:-}" ]]; then return 0; fi
    return 1; # False
}

#**************************************************************************************************
# @brief Deactivate current venv if active.
# @param None.
# @return 0 on success (or if none active), >0 on failure.
#**************************************************************************************************
venv_deactivate()
{
    # Fast-path: nothing to do
    if ! is_venv_active; then return 0; fi
    # 1) If pyenv has an explicit shell override, clear it first.
    # This can prevent "lingering" env selection even after deactivate. [1](https://bing.com/search?q=pyenv-virtualenv+deactivate+must+be+sourced+source+deactivate)
    if command -v pyenv >/dev/null 2>&1; then
        # If PYENV_VERSION is set, pyenv is forcing a version in this shell.
        if [[ -n "${PYENV_VERSION:-}" ]]; then pyenv shell --unset >/dev/null 2>&1 || true; fi
    fi
    # 2) Deactivate the Python venv layer (virtualenv/venv/pyenv-virtualenv).
    # For pyenv-virtualenv specifically, "deactivate must be sourced". [1](https://bing.com/search?q=pyenv-virtualenv+deactivate+must+be+sourced+source+deactivate)[2](https://stackoverflow.com/questions/43935610/why-cant-i-deactivate-pyenv-virtualenv-how-to-fix-installation)
    if [[ -n "${VIRTUAL_ENV:-}" ]]; then
        # If a deactivate *function* exists, use it (standard venv/virtualenv behavior).
        if declare -F deactivate >/dev/null 2>&1; then
            deactivate >/dev/null 2>&1 || true;
        # Otherwise, if a deactivate *command* exists, source it (pyenv-virtualenv guidance). [1](https://bing.com/search?q=pyenv-virtualenv+deactivate+must+be+sourced+source+deactivate)[2](https://stackoverflow.com/questions/43935610/why-cant-i-deactivate-pyenv-virtualenv-how-to-fix-installation)
        elif command -v deactivate >/dev/null 2>&1; then
            # shellcheck source=/dev/null
            source deactivate >/dev/null 2>&1 || true;
        fi
    fi
    # 3) Verify result (your original contract)
    if [[ -z "${VIRTUAL_ENV:-}" ]]; then return 0; fi
    log_msg "Error" "Failed to deactivate venv";
    paste_cmd "source deactivate";
    paste_cmd "deactivate";
    if command -v pyenv >/dev/null 2>&1; then
        paste_cmd "pyenv shell --unset";
        paste_cmd "pyenv shell system";
    fi
    return 1; # Fail
}

#**************************************************************************************************
# @brief Activate venv for global SWITCH_PROJECT (generic).
# @param None (uses global SWITCH_PROJECT).
# @return 0 on success, >0 on failure.
#**************************************************************************************************
venv_activate()
{
    if [[ -z "${SWITCH_PROJECT:-}" ]]; then log_msg "Error" "SWITCH_PROJECT is empty. Run --project project_name"; return 1; fi
    local pyexe=""; local py_prefix=""; local -i rc=0;
    local activate_path="${PATH_VENV:-}/${SWITCH_PROJECT:-}/bin/activate";
    # Enforce single method: venv only.
    if is_conda_active; then log_msg "Error" "Conda is active. Run: conda deactivate"; paste_cmd "conda deactivate"; return 2; fi
    # Step 1: already active & correct
    if is_venv_active; then
        if [[ "${VIRTUAL_ENV:-}/bin/activate" == "${activate_path}" ]]; then log_msg "Info" "Venv already active for '${SWITCH_PROJECT:-}'"; return 0; fi
        # Step 2: wrong venv active -> deactivate
        if ! venv_deactivate; then log_msg "Error" "Failed to deactivate existing venv: (${VIRTUAL_ENV:-}/bin/activate)"; return 3; fi
    fi
    # Step 3: activate
    if [[ ! -f "${activate_path}" ]]; then log_msg "Error" "Activation script not found: ${activate_path}"; paste_cmd "source ${activate_path}"; return 4; fi
    # shellcheck source=/dev/null
    if ! source "${activate_path}"; then rc=$?; log_msg "Error" "Failed to source (rc=${rc}): ${activate_path}"; paste_cmd "source ${activate_path}"; return 5; fi
    if [[ -z "${VIRTUAL_ENV:-}" ]]; then log_msg "Error" "VIRTUAL_ENV is empty after activation: ${activate_path}"; return 6; fi
    if [[ "${VIRTUAL_ENV}/bin/activate" != "${activate_path}" ]]; then log_msg "Error" "Activated wrong venv: expected ${activate_path}, got ${VIRTUAL_ENV}"; return 7; fi
    # Validate python
    pyexe="$(command -v python 2>/dev/null || true)";
    if [[ -z "${pyexe}" ]]; then log_msg "Error" "python not found after activation"; return 8; fi
    if [[ "${pyexe}" != "${VIRTUAL_ENV}/bin/python" ]]; then log_msg "Error" "python mismatch: expected ${VIRTUAL_ENV}/bin/python, got ${pyexe}"; return 9; fi
    py_prefix="$(python -c 'import sys; print(sys.prefix)' 2>/dev/null || true)";
    if [[ -z "${py_prefix}" ]]; then log_msg "Error" "python failed to run after activation"; return 10; fi
    if [[ "${py_prefix}" != "${VIRTUAL_ENV}" ]]; then log_msg "Error" "sys.prefix mismatch: expected ${VIRTUAL_ENV}, got ${py_prefix}"; return 11; fi
    log_msg "Pass" "Activated venv for '${SWITCH_PROJECT:-}': ${VIRTUAL_ENV}";
    paste_cmd "source ${activate_path}";
    return 0; # Pass
}

#****************************************************************
# @brief      Copy a source file into the startup backup date folder
#             while preserving directory structure relative to PATH_ROOT.
#             Destination is always under PATH_BACKUPS_STARTUP_RUN.
#             Uses rsync --relative (-R) to preserve tree automatically.
#             Skips files already under PATH_BACKUPS_STARTUP to prevent recursion.
# @param      $1 source_file Absolute path to the file to backup.
# @return     0 on success (including skip), 1 on failure, 2 on usage error.
#****************************************************************
backup2startup()
{
    local source_file; source_file="${1:-}";
    local rel_dir;
    local dest;

    if [[ -z "${source_file}" ]]; then log_msg "Error" "[${FUNCNAME[0]}] Missing source_file argument"; return 2; fi
    if [[ -z "${PATH_ROOT:-}" ]]; then log_msg "Error" "[${FUNCNAME[0]}] PATH_ROOT must be set (global)"; return 2; fi
    if [[ -z "${PATH_BACKUPS_STARTUP:-}" ]]; then log_msg "Error" "[${FUNCNAME[0]}] PATH_BACKUPS_STARTUP must be set (global)"; return 2; fi
    if [[ "${source_file}" != /* ]]; then log_msg "Error" "[${FUNCNAME[0]}] source_file must be absolute: ${source_file}"; return 2; fi
    if [[ ! -f "${source_file}" ]]; then log_msg "Error" "[${FUNCNAME[0]}] Source file not found: ${source_file}"; return 1; fi
    if [[ ! -r "${source_file}" ]]; then log_msg "Error" "[${FUNCNAME[0]}] Source file not readable: ${source_file}"; return 1; fi
    if [[ "${source_file}" == "${PATH_BACKUPS_STARTUP}"/* ]]; then log_msg "Warning" "[${FUNCNAME[0]}] SKIP: source already under Backups: ${source_file}"; return 0; fi
    if [[ "${source_file}" != "${PATH_ROOT}"/* ]]; then log_msg "Error" "[${FUNCNAME[0]}] Source file is not under PATH_ROOT: ${source_file}"; return 1; fi
    if ! command -v rsync >/dev/null 2>&1; then log_msg "Error" "[${FUNCNAME[0]}] Required command not found: rsync"; return 1; fi
    if ! check_backup_folder; then log_msg "Error" "[${FUNCNAME[0]}] Backup folder init failed"; return 1; fi
    if [[ -z "${PATH_BACKUPS_STARTUP_RUN:-}" ]]; then log_msg "Error" "[${FUNCNAME[0]}] PATH_BACKUPS_STARTUP_RUN not initialized"; return 1; fi

    # Compute relative directory under PATH_ROOT
    rel_dir="$(dirname "${source_file#"${PATH_ROOT}"/}")"
    filename="$(basename "$source_file")"

    # Full expected destination file path
    dest="${PATH_BACKUPS_STARTUP_RUN}/${rel_dir}/${filename}"

    # Use PATH_ROOT as rsync root so -R preserves the correct structure
    (
        cd "${PATH_ROOT}" || exit 1
        # log_msg "Warning" "rsync -aR ./\"${rel_dir}/${filename}\" ${PATH_BACKUPS_STARTUP_RUN}/"
        if ! rsync -aR -- "./${rel_dir}/${filename}" "${PATH_BACKUPS_STARTUP_RUN}/"; then
            log_msg "Error" "Fail rsync source: [${source_file}] dest: [${PATH_BACKUPS_STARTUP_RUN}/]"
            exit 1
        fi
    )

    # Verify
    if [[ ! -f "${dest}" ]]; then
        log_msg "Error" "Destination missing after rsync: ${dest}"
        return 1
    fi

    #if [[ ! -f "${dest}" ]]; then log_msg "Error" "[${FUNCNAME[0]}] Destination missing after rsync: ${dest}"; return 1; fi

    if [[ -s "${source_file}" && ! -s "${dest}" ]]; then
        log_msg "Error" "[${FUNCNAME[0]}] Destination empty but source not: ${dest}"; return 1;
    fi

    return 0; # Pass
}

#**************************************************************************************************
# @brief      Check a source file (chmod, WSL dos2unix, lint/syntax).
# @param      $1 source_file full path.
#**************************************************************************************************
check_source_file()
{
    # Check argument count
    if (( $# != 2 )); then log_msg "Error" "Usage: ${FUNCNAME[0]} <source_file> <pylint level: 1 to 3>"; return 1; fi
    # Get argument
    local source_file; source_file="${1}";
    if [[ -z "${source_file}" ]]; then log_msg "Error" "${FUNCNAME[0]}: source_file argument is empty"; return 1; fi
    # Check if file exists
    if [[ ! -f "${source_file}" ]]; then log_msg "Error" "File not found: (${source_file})"; return 1; fi
    # Get file extension
    local ext; ext="${source_file##*.}";
    log_msg "Info" "Check source file [${source_file}]";
    #
    declare -i py_lint_level="${2}"; # Higher is stricter
    if [[ -z "${py_lint_level}" ]]; then log_msg "Error" "${FUNCNAME[0]}: source_file argument py_lint_level is empty"; return 1; fi
    #-----------------------------
    # Always chmod +x
    #-----------------------------
    if ! chmod +x "${source_file}"; then log_msg "Error" "Failed to chmod +x ${source_file}"; fi
    #-----------------------------
    # Backup to txt
    #-----------------------------
    if ! backup2startup "${source_file}"; then log_msg "Error" "Failed to copy: ${source_file} to ${source_file}.txt"; return 1; fi
    #-----------------------------
    # Detect OS
    #-----------------------------
    local -i is_wsl=0;
    # local -i is_linux=0;
    # local -i is_macos=0;
    if grep -qi "microsoft" /proc/version 2>/dev/null; then is_wsl=1; fi
    # if [[ "$(uname -s)" == "Linux" ]];  then is_linux=1; fi
    # if [[ "$(uname -s)" == "Darwin" ]]; then is_macos=1"; fi
    #-----------------------------
    # WSL: run dos2unix for .sh files
    #-----------------------------
    if (( is_wsl == 1 )) && [[ "${ext}" == "sh" ]]; then
        # Run dos2unix on DOS machines to fix New Lines, Line returns and so on.
        if command -v dos2unix >/dev/null 2>&1; then
            if ! dos2unix -q "${source_file}" >/dev/null 2>&1; then log_msg "Error" "dos2unix failed for: ${source_file}"; return 1; fi
        else
            log_msg "Warning" "dos2unix not installed; skipping";
            # FIXME install it
        fi
    fi
    #-----------------------------
    # Linux/macOS: no dos2unix
    #-----------------------------
    # if (( is_linux == 1 )) || (( is_macos == 1 )); then if [[ "${ext}" == "sh" ]]; then log_msg "Info" "Running shellcheck + bash -n on: ${source_file}";  fi fi
    #-----------------------------
    # Linting / Syntax
    #-----------------------------
    if [[ "${ext}" == "sh" ]]; then
        # shell check test
        if ! shellcheck "${source_file}"; then log_msg "Error" "ShellCheck failed for: ${source_file}"; return 1; fi
        # bash -n test
        if ! bash -n "${source_file}"; then log_msg "Error" "bash -n failed for: ${source_file}"; return 1; fi
    elif [[ "${ext}" == "py" ]]; then
        if ! checkpy "${source_file}" "${py_lint_level}"; then log_msg "Error" "Failed checkpy on: ${source_file} with Py Lint level: ${py_lint_level}"; return 1; fi
    fi
    # FIXME add extended testing for encoding
    return 0; # Pass
}

#**************************************************************************************************
# @brief      Source all Bash libs and validate Python helpers and encodable text files.
#**************************************************************************************************
check_source()
{
    show_header "${MAGENTA}";
    log_msg "Notice" "Checking source file: shellcheck, Python Lint, and encoding standards...";
    local file;
    # Check Source Files Library
    for file in "${SOURCE_SH_LIBRARY[@]}"; do
        if ! check_source_file "${file}" "${PYLINT_LEVEL_DEFAULT}"; then
            log_msg "Error" "Failed check source library files: ${file}";
            return 1;
        fi
    done
    # Check Source Files Shared
    for file in "${SOURCE_SH_SHARED[@]}"; do
        if ! check_source_file "${file}" "${PYLINT_LEVEL_DEFAULT}"; then
            log_msg "Error" "Failed check shared source shared files: ${file}";
            return 1;
        fi
    done
    # Check Source Files Project
    for file in "${SOURCE_PROJECT_SH[@]}"; do
        if ! check_source_file "${file}" "${PYLINT_LEVEL_DEFAULT}"; then
            log_msg "Error" "Failed check source project files: ${file}";
            return 1;
        fi
    done
    # Check Source Files Root
    for file in "${SOURCE_MAIN_SH[@]}"; do
        if ! check_source_file "${file}" "${PYLINT_LEVEL_DEFAULT}"; then
            log_msg "Error" "Failed check source root files: ${file}";
            return 1;
        fi
    done
    # Check encoding
    for file in "${SOURCE_ENCODING[@]}"; do
        if ! clean_encoding "${file}"; then
            log_msg "Error" "Failed files encoding: ${file}";
            return 1;
        fi
    done
    log_msg "Pass" "Pass: Source check";
    # Show Footer
    show_footer "${MAGENTA}";
    return 0; # Pass
}

#**************************************************************************************************
# @brief      Fix encoding issues in configured files.
# @param      $1 file path to clean (one file per call; the calling loop iterates).
#**************************************************************************************************
clean_encoding()
{
    if (( $# != 1 )); then log_msg "Error" "${FUNCNAME[0]} requires exactly one file path argument"; return 1; fi
    # Check arg
    local file; file="${1}";
    if [[ ! -f "${file}" ]]; then log_msg "Error" "Required file missing: ${file}. Aborting."; return 1; fi
    # Detect if cleaning is required
    local -i needs_clean=0;
    if grep -q $'\r' "${file}"; then needs_clean=1; fi
    if grep -q '[[:space:]]$' "${file}"; then needs_clean=1; fi
    if grep -q '[^[:print:][:space:]]' "${file}"; then needs_clean=1; fi
    # Return if clean is not required
    if (( needs_clean == 0 )); then return 0; fi
    # Log cleaning
    log_msg "Info" "Cleaning encoding for: ${file}";
    if ! cp "${file}" "${file}.bak"; then log_msg "Error" "Failed to create backup for ${file}. Check permissions."; return 1; fi
    # Normalize special characters and trim trailing whitespace.
    #   Note: the [[:space:]]*$ pattern removes trailing spaces/tabs.
    sed -i \
        -e 's/>/\>/g' \
        -e 's/</\</g' \
        -e 's/&/\\&/g' \
        -e 's/[[:space:]]*$//' \
        "${file}";

    local temp_file; temp_file="$(mktemp)" || { log_msg "Error" "Could not create temp file."; return 1; };
    tr -cd '\11\12\15\40-\176' < "${file}" > "${temp_file}";
    if ! mv "${temp_file}" "${file}"; then log_msg "Error" "Failed to move cleaned temp file into place."; return 1; fi
    # Check if file is empty
    if [[ ! -s "${file}" ]]; then log_msg "Error" "File ${file} is empty after cleaning. Aborting."; return 1; fi
    #************************************************************
    run_dos2unix()
    {
        local file; file="${1}";
        if command -v dos2unix >/dev/null 2>&1; then
            if dos2unix -q "${file}"; then
                return 0;
            else
                log_msg "Error" "Failed: dos2unix";
                return 1;
            fi
        fi
    }
    # Only run if WSL
    if grep -qi "microsoft" /proc/version 2>/dev/null; then
        if ! run_dos2unix "${file}"; then
            if os_update_install; then
                if ! run_dos2unix "${file}"; then
                    log_msg "Failed to update to install dos2unit";
                    return 1; # Fatal error of no return
                fi
            else
                return 1; # Fatal error of no return
            fi
        fi

    fi
    return 0; # Pass
}

#**************************************************************************************************
# @brief   Check for unexpected folders under PATH_BASH.
# @details Compares actual folders against VALID_PATH_ROOTS_SH allow-list and reports extras.
#          This helps catch layout drift and hidden bug sources early.
# @param   none
# @return  0 (non-fatal; warnings only)
#**************************************************************************************************
check_allowed_folders()
{
    local d=""; local name="";
    local -A allowed=(); local allowed_path="";

    if [[ -z "${PATH_BASH:-}" ]]; then
        log_msg "Error" "PATH_BASH is not defined";
        return 1;
    fi
    if [[ ! -d "${PATH_BASH}" ]]; then
        log_msg "Error" "PATH_BASH not found: ${PATH_BASH}";
        return 1;
    fi
    if [[ -z "${VALID_PATH_ROOTS_SH+set}" ]]; then
        log_msg "Error" "VALID_PATH_ROOTS_SH not defined (source source_files.sh)";
        return 1;
    fi

    # Build allow-list by basename
    for allowed_path in "${VALID_PATH_ROOTS_SH[@]}"; do
        allowed["$(basename -- "${allowed_path}")"]=1;
    done

    # Scan direct children only
    (
        shopt -s nullglob dotglob;
        for d in "${PATH_BASH}"/*; do
            if [[ -d "${d}" ]]; then
                name="$(basename -- "${d}")";
                if [[ -z "${allowed[${name}]+set}" ]]; then
                    log_msg "Notice" "Unexpected folder under PATH_BASH: ${name} (add to VALID_PATH_ROOTS_SH if intentional)";
                else
                    log_msg "Notice" "Allowed folder: ${name}";
                fi
            fi
        done
    );

    return 0; # Pass
}

#****************************************************************
# @brief      Create .pylintrc ONLY if missing. Never overwrite.
#             Uses external template files for levels 1,2,3.
# @param      $1: level (1,2,3)
# @return     0 on success, 1 on failure.
#****************************************************************
set_pylintrc()
{
    local level="${1:-}"
    local root="${PATH_ROOT:-$(pwd)}"
    local chosen="${root}/.pylintrc"
    local template_dir="${SCRIPT_DIR}/pylint"
    local template=""

    # Validate level
    if [[ -z "${level}" ]]; then
        log_msg "Error" "[${FUNCNAME[0]}] Missing level (1..3)"
        return 1
    fi
    if [[ "${level}" != "1" && "${level}" != "2" && "${level}" != "3" ]]; then
        log_msg "Error" "[${FUNCNAME[0]}] Invalid level '${level}' (expected 1..3)"
        return 1
    fi

    # Select template file
    template="${template_dir}/pylintrc.level${level}"
    if [[ ! -f "${template}" ]]; then
        log_msg "Error" "[${FUNCNAME[0]}] Missing template: ${template}"
        return 1
    fi

    # Do NOT overwrite existing .pylintrc
    if [[ -f "${chosen}" ]]; then
        log_msg "Notice" "[${FUNCNAME[0]}] ${chosen} already exists — not overwriting"
        return 0
    fi

    # Create .pylintrc from template
    log_msg "Notice" "[${FUNCNAME[0]}] Creating ${chosen} from ${template}"
    cp "${template}" "${chosen}"

    # Ensure final newline
    if [[ -s "${chosen}" ]]; then
        local last_byte
        last_byte="$(tail -c 1 -- "${chosen}" 2>/dev/null || true)"
        [[ "${last_byte}" != $'\n' ]] && printf "\n" >> "${chosen}"
    fi

    log_msg "Pass" "[${FUNCNAME[0]}] Installed .pylintrc (level ${level})"
    return 0
}

#**************************************************************************************************
# @brief      Lint a Python file using pylint. Uses project venv and installs pylint if missing.
# @param      $1 source_file (python file)
# @param      $2 lint level (1..3)
# @return     0 on success, non-zero on failure.
#**************************************************************************************************
checkpy()
{
    if (( $# != 2 )); then log_msg "Error" "Usage: ${FUNCNAME[0]} <source_file> <lint level>"; return 1; fi
    local py_file; py_file="${1:-}";
    local py_lint_level; py_lint_level="${2:-}";
    if [[ -z "${py_file}" ]]; then log_msg "Error" "Missing py_file"; return 1; fi
    if [[ -z "${py_lint_level}" ]]; then log_msg "Error" "Missing py_lint_level"; return 1; fi
    if [[ "${py_lint_level}" != "1" && "${py_lint_level}" != "2" && "${py_lint_level}" != "3" ]]; then
        log_msg "Error" "Invalid lint level '${py_lint_level}' (expected 1..3)"; return 1;
    fi
    if [[ ! -f "${py_file}" ]]; then log_msg "Error" "File not found: ${py_file}"; return 1; fi
    local -i rc=0;
    if ! venv_activate; then
        log_msg "Error" "Failed [${FUNCNAME[0]}] Did not find activate path for this project"; return 1;
    fi

    if [[ -z "${PYTHON_COMMAND:-}" || ! -x "${PYTHON_COMMAND}" ]]; then
        log_msg "Error" "PYTHON_COMMAND missing or not executable: ${PYTHON_COMMAND:-<empty>}"; return 1;
    fi
    if ! set_pylintrc "${py_lint_level}"; then log_msg "Error" "Failed set_pylintrc level ${py_lint_level}"; return 1; fi
    local root; root="${PATH_ROOT:-$(pwd)}";
    local rcfile; rcfile="${root}/.pylintrc";
    if [[ ! -f "${rcfile}" ]]; then log_msg "Error" ".pylintrc not found after set_pylintrc: ${rcfile}"; return 1; fi
    log_msg "Info" "Check PY: (${SCRIPT_VERSION_MAIN}) ${SCRIPT_DATE_MAIN}";
    log_msg "Info" "Python interpreter: ${PYTHON_COMMAND}";
    # Ensure pip exists in venv
    if ! "${PYTHON_COMMAND}" -m pip --version >/dev/null 2>&1; then log_msg "Error" "pip missing in venv: ${PYTHON_COMMAND}"; return 1; fi
    # Ensure pylint installed in venv (NO --user in venv)
    if ! "${PYTHON_COMMAND}" -m pip show pylint >/dev/null 2>&1; then
        log_msg "Info" "Installing pylint into venv...";
        if ! "${PYTHON_COMMAND}" -m pip install "$(pip_verbosity_flag 2>/dev/null || printf "%s" "--quiet")" pylint; then
            log_msg "Error" "Failed to install pylint in venv";
            return 1;
        fi
    fi
    # Run pylint using selected rcfile
    if ! "${PYTHON_COMMAND}" -m pylint --rcfile="${rcfile}" "${py_file}"; then
        log_msg "Error" "pylint failed: ${py_file}";
        return 1;
    fi
    log_msg "Pass" "Passed checkpy (level ${py_lint_level}): ${py_file}";
    return 0; # Pass
}

#**************************************************************************************************
# @brief      Ensure required file exists (utility).
#**************************************************************************************************
require_file()
{
    local file; file="${1}";
    if [[ -z "${file}" ]];   then log_msg "Error" "[${FUNCNAME[0]} Missing file argument]"; return 1; fi
    if [[ ! -f "${file}" ]]; then log_msg "Error" "Required file missing: |${file}|"; return 1; fi
    return 0; # Pass
}

#**************************************************************************************************
# @brief      Trace back calls in the Bash stack.
# @param      depth Number of frames to trace back (>0).
#**************************************************************************************************
trace_calls()
{
    local -i depth; depth=20;   # 20 is deep
    local color_text="${GRAY}"; # WHITE is too bright
    local spacing="    ←";
    # Sanity check
    if (( depth <= 0 )); then
        printf "%b%b Invalid call to trace_calls <depth>. Depth must be greater than 0 %b \n" "${color_text}" "⚠️" "${NC}";
        return 1;
    fi
    # Setup
    local -i i; i=0;
    local -i max_depth; max_depth=$(( ${#BASH_SOURCE[@]} - 1 ));
    local trace; trace="";
    # Reality check
    if (( depth > max_depth )); then depth="${max_depth}"; fi
    # Trace it
    while (( i < depth )); do
        local script;  script="$(basename "${BASH_SOURCE[$((i+1))]:-$0}")";
        local func;    func="${FUNCNAME[$((i+1))]:-MAIN}";
        local -i line; line="${BASH_LINENO[$i]:-0}";
        if (( line == 1 )) && [[ "${func}" != "MAIN" ]]; then line="?"; fi
        # Add newline if not first line
        if [[ -n "${trace:-}" ]]; then trace+=$'\n'; fi
        # Add spacing prefix to every line
        trace+="${spacing} ${script}->${func}->${line}";
        i=$((i + 1));
    done

    printf "<<< trace_calls: >>>\n%s\n" "${trace}";
    return 0; # Pass
}

#**************************************************************************************************
# @brief      Print timestamp in selected format.
# @param      $1 "HMS" or "YMD-HMS"
#**************************************************************************************************
timestamp()
{
    local format_dt; format_dt="${1:-}";
    if [[ -z "${format_dt}" ]]; then
        printf "timestamp: missing required format (expected HMS or YMD-HMS)\n" >&2; return 2;
    fi
    case "${format_dt}" in
        "HMS")     date '+%H:%M:%S'; ;;
        "YMD-HMS") date '+%Y-%m-%d %H:%M:%S'; ;;
        *)         printf "timestamp: invalid format %s (allowed: HMS, YMD-HMS)\n" "${format_dt}" >&2; return 2; ;;
    esac;
    return 0; # Pass
}

#**************************************************************************************************
# @brief      Format seconds to HH:MM:SS.
#**************************************************************************************************
fmt_hms()
{
    local s; s="${1:-0}";
    (( s < 0 )) && s=0;
    printf "%02d:%02d:%02d" $((s/3600)) $(((s%3600)/60)) $((s%60));
    return 0; # Pass
}

#**************************************************************************************************
# @brief      Icon for a given log level (single-width).
#**************************************************************************************************
level_icon()
{
    local level; level="${1:-}"; # (Pass|Info|Warning|Error|Debug|Header|Delta|Steps|Text|Trace)
    local mode;  mode="${LOG_ICONS:-unicode}";
    if [[ "${mode}" == "ascii" ]]; then
        case "${level}" in
            pass)    printf "[OK]"; ;;  # 1.
            success) printf "[^]"; ;;   # 2. success
            info)    printf "[i]"; ;;   # 3.
            notice)  printf "[N]"; ;;   # 4.
            warning) printf "[!]"; ;;   # 5.
            error)   printf "[X]"; ;;   # 6.
            debug)   printf "[D-]"; ;;  # 7. Debug:   set -x enable debug
            nodebug) printf "[D+]"; ;;  # 8. NoDeBug: set +x disable debug
            header)  printf "[H]"; ;;   # 9.
            delta)   printf "[T]"; ;;   # 10.
            nodelta) printf "[?]"; ;;   # 11. No delta
            steps)   printf "[S]"; ;;   # 12.
            text)    printf "[@]"; ;;   # 13.
            trace)   printf "[>]"; ;;   # 14.
            *)       printf "[ ]"; ;;   #
        esac
        return 0;
    fi
    case "${level}" in
        pass)    printf "✔"; ;; # 1. Pass
        success) printf "✔"; ;; # 2. success
        info)    printf "ℹ"; ;;  # 3. Info: i or ℹ, ⓘ,
        notice)  printf "△"; ;; # 4. Notice
        warning) printf "▲"; ;;  # 5. Warning
        error)   printf "✖"; ;;  # 6. Error
        debug)   printf "▶"; ;;  # 7. Debug
        nodebug) printf "▷"; ;;  # 8. NoDebug
        header)  printf "■"; ;;  # 9. Header
        delta)   printf "⌚"; ;; # 10. Delta: ⌚,⌛,⏳,⏲,⏱,⏰,🕒
        nodelta) printf "∅"; ;;  # 11. No delta
        steps)   printf "→"; ;;  # 12. Steps (narrow): ➜
        text)    printf "≡"; ;;  # 13. Text
        trace)   printf "↳"; ;;  # 14. Trace
        *)       printf "·"; ;;  # Unknown
    esac
    return 0; # Pass
}

#**************************************************************************************************
# @brief      Color for a given log level.
# @parm       level
#**************************************************************************************************
level_color()
{
    local level; level="${1}"; # (Pass|Info|Warning|Error|Debug|Header|Delta|Steps|Text|Trace)
    case "${level}" in
        pass)    printf "%b" "${GREEN}"; ;;     # 1. Pass
        success) printf "%b" "${LIME}"; ;;      # 2. Success
        info)    printf "%b" "${CYAN}"; ;;      # 3. Info
        notice)  printf "%b" "${GRAY}"; ;;      # 4. Notice
        warning) printf "%b" "${YELLOW}"; ;;    # 5. Warning
        error)   printf "%b" "${BRIGHTRED}"; ;; # 6. Error: RED
        debug)   printf "%b" "${MAGENTA}"; ;;   # 7. Debug:   set -x enable debug
        nodebug) printf "%b" "${MAGENTA}"; ;;   # 8. NoDeBug: set +x disable debug
        header)  printf "%b" "${SKYBLUE}"; ;;   # 9. Header
        delta)   printf "%b" "${BLUE}"; ;;      # 10. Delta
        nodelta) printf "%b" "${SKYBLUE}"; ;;   # 11. No delta
        steps)   printf "%b" "${GRAY}"; ;;      # 12. Steps
        text)    printf "%b" "${WHITE}"; ;;     # 13. Text
        trace)   printf "%b" "${ORANGE}"; ;;    # 14. Trace
        *)       printf "%b" "${NC}"; ;;        # Normal Color
    esac
    return 0; # Pass
}

#**************************************************************************************************
# @brief      Clean Log message.
# @example    if (( $# != 1 )); then debug_missing_arg "missing_arg" "pylint level: 1 to 3"; return 1; fi
#**************************************************************************************************
debug_missing_arg()
{
    local hint_1="Missing Argument"; local hint_2="Explains how to fix it";
    if (( $# != 2 )); then printf "%b Usage: %b %s '%s' (%s) %b \n" "${RED}" "${WHITE}" "${FUNCNAME[1]}" "${hint_1}" "${hint_2}" "${NC}"; return 1; fi
    local missing_arg="${1}";
    if [[ -z "${missing_arg}" ]]; then printf "%b Missing:%b (%s) %b \n" "${RED}" "${WHITE}" "${hint_1}" "${NC}"; return 1; fi
    local hint="${2}";
    if [[ -z "${hint}" ]]; then printf "%b Missing:%b (%s) %b \n" "${RED}" "${WHITE}" "${hint_2}" "${NC}"; return 1; fi
    printf "%b Usage:%b %s '%s' (%s) %b \n" "${RED}" "${WHITE}" "${FUNCNAME[1]}" "${missing_arg}" "${hint}" "${NC}";
    return 1; # Fail
}

#**************************************************************************************************
# @brief      Clean Log message.
# @example:
#        Path: /home/username/path/file.ext
#        [sudo] password for username:
#        username@DesktopName:~/Folder$
# @param      $1 Message.
# @return     Clean Message with /home/${USER} -> ${HOME} and username -> ${USER}.
#**************************************************************************************************
clean_log_msg()
{
    if (( $# != 1 )); then debug_missing_arg "missing_arg" "pylint level: 1 to 3"; return 1; fi
    local msg2clean; msg2clean="${1}";
    if [[ -z "${msg2clean}" ]]; then log_msg "Error" "Missing argument msg2clean"; return 1; fi
    #local user; user="${USER}";
    local clean_msg;
    clean_msg="$(printf "%s" "${msg2clean}" | perl -pe '
        my $u = $ENV{"USER"};

        # Replace /home/<user> or /home/<user>/...
        s|/home/\Q$u\E(/?)|\${HOME}$1|g;

        # Replace [sudo] password for <user>:
        s/\[sudo\] password for \Q$u\E:/[sudo] password for \${USER}:/g;

        # Replace <user>@<host>:
        s/(^|[^[:alnum:]_])\Q$u\E@[A-Za-z0-9_.-]+:/$1\${USER}@\${HOSTNAME}:/g;

        # Replace remaining standalone <user>
        s/(^|[^[:alnum:]_])\Q$u\E([^[:alnum:]_]|$)/$1\${USER}$2/g;
    ')";
    printf "%s" "${clean_msg}";
    return 0; # Pass
}

#**************************************************************************************************
# @brief      Log message with timestamp + delta.
# @param      $1 Level (Pass|Info|Warning|Error|Debug|Header|Delta|NoDeBug)
# @param      $2 Message
#**************************************************************************************************
log_msg()
{
    local level; level="${1}"; level="${level,,}"; # Normalize to lowercase (no CamelCase matching)
    local msg;   msg="${2}";
    if [[ -z "${msg}" ]]; then printf "Error Missing argument msg"; return 1; fi
    msg="$(clean_log_msg "${msg}")";
    #----------------------------------------------------------------------
    # Caller context (IMMEDIATE caller)
    #----------------------------------------------------------------------
    local -a src_arr;  src_arr=("${BASH_SOURCE[@]:-}");
    local -a fn_arr;   fn_arr=("${FUNCNAME[@]:-}");
    local -a line_arr; line_arr=("${BASH_LINENO[@]:-}");
    local caller_src;  caller_src="${src_arr[1]:-${0}}"; caller_src="${caller_src##*/}";
    local caller_fn;   caller_fn="${fn_arr[1]:-MAIN}";
    local caller_ln;   caller_ln="${line_arr[0]:-0}";
    #----------------------------------------------------------------------
    # Visuals
    #----------------------------------------------------------------------
    local color; color="$(level_color "${level}")";
    local ts;    ts="$(timestamp HMS)";
    local now;   now="$(date +%s)";
    local -i delta_sec=0;
    if [[ -n "${ET_START_TIME:-}" ]]; then
        delta_sec=$(( now - ET_START_TIME ));
        (( delta_sec < 0 )) && delta_sec=0;
    fi
    ET_START_TIME="${now}";
    local delta; delta="$(fmt_hms "${delta_sec}")";
    local delta_color="${color}";
    if   (( delta_sec > 1   && delta_sec <= 30 )); then delta_color="${GREEN}";
    elif (( delta_sec > 30  && delta_sec <= 60 )); then delta_color="${YELLOW}";
    elif (( delta_sec > 60  && delta_sec <= 300)); then delta_color="${MAGENTA}";
    elif (( delta_sec > 300 ));                    then delta_color="${RED}";
    fi
    local icon; icon="$(level_icon "${level}")";
    local icon_delta; icon_delta=$([[ "${delta_sec}" -gt 1 ]] && printf "∆" || printf "∅");
    #----------------------------------------------------------------------
    # Trace prefix (opt‑in)
    # Recommendation:
    #  - Trace should be lightweight, never full stack
    #----------------------------------------------------------------------
    if (( SWITCH_TRACE == 1 )) || [[ "${level}" == "trace" ]]; then
        msg="[${caller_src}:${caller_fn}:${caller_ln}] ${msg}";
    fi
    #----------------------------------------------------------------------
    # Warning / Error / Debug stack formatting
    # Recommendation:
    #  - Only Error / Debug / NoDeBug show stack
    #  - File-first format for grep & CI readability
    #----------------------------------------------------------------------
    if [[ "${level}" == "warning" ]]; then
        msg="[${caller_src}:${caller_fn}:${caller_ln}] ${msg}";
    elif [[ "${level}" == "notice" ]]; then
        msg=">>> ${msg}";
    elif [[ "${level}" == "debug" || "${level}" == "error" || "${level}" == "nodebug" ]]; then
        local arrow=$'\u2190';
        local nl=$'\n';
        local spacer="      ";
        local depth="${#fn_arr[@]}";
        local stack="";
        # Build stack: caller → … → MAIN
        for (( i=1; i<depth; i++ )); do
            local src="${src_arr[i]:-${0}}"; src="${src##*/}";
            local fn="${fn_arr[i]:-MAIN}";
            local ln="${line_arr[i-1]:-0}";
            local frame="[${src}:${fn}:${ln}]";
            if [[ -z "${stack}" ]]; then
                stack="${frame}";
            else
                stack="${stack}${nl}${spacer}${arrow} ${frame}";
            fi
        done
        # Append MAIN frame
        local main_src="${src_arr[depth-1]:-${0}}"; main_src="${main_src##*/}";
        local main_ln="${line_arr[depth-1]:-0}";
        stack="${stack}${nl}${spacer}${arrow} [${main_src}:MAIN:${main_ln}]";
        #
        if is_venv_active; then
            msg="[${caller_src}:${caller_fn}:${caller_ln}]${nl} ${msg}${nl} VENV=${VIRTUAL_ENV}${nl} ${spacer}STACK: ${stack}";
        else
            msg="[${caller_src}:${caller_fn}:${caller_ln}]${nl} ${msg}${nl} |${nl}${spacer}STACK: ${stack}";
        fi
        #
        # Debug toggles (kept explicit)
        if   [[ "${level}" == "debug" ]];   then set -x;
        elif [[ "${level}" == "nodebug" ]]; then set +x;
        fi
    fi
    #----------------------------------------------------------------------
    # Emit to console
    #----------------------------------------------------------------------
    printf "%b %s [%b%s %s %s%b] %s %b\n" "${color}" "${icon}" "${delta_color}" "${ts}" "${icon_delta}" "${delta}" "${color}" "${msg}" "${NC}";
    #----------------------------------------------------------------------
    # Emit to log file (plain, grep‑friendly)
    #----------------------------------------------------------------------
    if [[ -n "${LOG_FILE:-}" ]]; then printf " %s [%s %s %s] %s\n" "${icon}" "${ts}" "${icon_delta}" "${delta}" "${msg}" >> "${LOG_FILE}"; fi
    return 0; # Pass
}

#**************************************************************************************************
# @brief      Show the defined color names.
#**************************************************************************************************
show_colors()
{
    printf "%b *********%b*********%b*********%b*********%b*********%b*********%b*********%b*********%b*********%b******* %b \n" "${GREEN}" "${LIME}" "${YELLOW}" "${ORANGE}" "${MAGENTA}" "${RED}" "${BRIGHTRED}" "${BLUE}" "${SKYBLUE}" "${CYAN}" "${NC}";
    printf "%b 12 Colors: %b \n" "${CYAN}" "${NC}";
    local w=12;
    # Line 1
    printf "%b %-*s%b"   "${GREEN}"   "${w}" "GREEN"     "${NC}";
    printf "%b%-*s%b\n" "${LIME}"    "${w}" "LIME"      "${NC}";
    # Line 2
    printf "%b %-*s%b"   "${YELLOW}"  "${w}" "YELLOW"    "${NC}";
    printf "%b%-*s%b\n" "${ORANGE}"  "${w}" "ORANGE"    "${NC}";
    # Line 3
    printf "%b %-*s%b"   "${MAGENTA}" "${w}" "MAGENTA"   "${NC}";
    printf "%b%-*s%b"   "${RED}"     "${w}" "RED"       "${NC}";
    printf "%b%-*s%b\n" "${BRIGHTRED}" "${w}" "BRIGHTRED" "${NC}";
    # Line 4
    printf "%b %-*s%b"   "${BLUE}"    "${w}" "BLUE"      "${NC}";
    printf "%b%-*s%b"   "${SKYBLUE}" "${w}" "SKYBLUE"   "${NC}";
    printf "%b%-*s%b\n" "${CYAN}"    "${w}" "CYAN"      "${NC}";
    # Line 5
    printf "%b %-*s%b"   "${GRAY}"    "${w}" "GRAY"      "${NC}";
    printf "%b%-*s%b\n" "${WHITE}"   "${w}" "WHITE"     "${NC}";
    return 0; # Pass
}

#**************************************************************************************************
# @brief      Test all interfaces.
#**************************************************************************************************
run_test_interface()
{
    show_header "${GRAY}";
    local message_showing="This is to log_msg levels";
    run_level_test()
    {
        log_msg "${1}" "${message_showing}: ${1}";
    }
    # Run test for all legal values.
    # Must maintain this list
    printf "%b Testing log_msg Pass %b\n" "${WHITE}" "${NC}";
    run_level_test "Pass";
    printf "%b Testing log_msg Success %b\n" "${WHITE}" "${NC}";
    run_level_test "Success";
    printf "%b Testing log_msg Info %b\n" "${WHITE}" "${NC}";
    run_level_test "Info";
    printf "%b Testing log_msg Header %b\n" "${WHITE}" "${NC}";
    run_level_test "Header";
    printf "%b Testing log_msg Steps %b\n" "${WHITE}" "${NC}";
    run_level_test "Steps";
    printf "%b Testing log_msg Text %b\n" "${WHITE}" "${NC}";
    run_level_test "Text";
    printf "%b Testing log_msg Delta %b\n" "${WHITE}" "${NC}";
    run_level_test "Delta";
    printf "%b Testing log_msg Trace %b\n" "${WHITE}" "${NC}";
    run_level_test "Trace";
    printf "%b Testing log_msg Warning %b\n" "${WHITE}" "${NC}";
    run_level_test "Warning";
    printf "%b Testing log_msg Error %b\n" "${WHITE}" "${NC}";
    run_level_test "Error";
    printf "%b Testing log_msg Debug %b\n" "${WHITE}" "${NC}";

    if [[ -n "${PATH_VENV:-}" ]]; then
    if [[ -z "${PATH_VENV:-}" ]]; then declare -gx PATH_VENV="${PATH_ROOT}/${NAME_FOLDER_VENV}"; fi
    fi

    run_level_test "Error";
    printf "%b Testing log_msg Debug %b\n" "${WHITE}" "${NC}";

    run_level_test "Debug";
    local -i test_this=0;
    if (( test_this == 0 )); then printf "%b Debugging... %b \n" "${WHITE}" "${NC}"; fi
    set +x; # Turn off debug mode
    printf "%b Testing log_msg NoDeBug. Turned off debug. This does the same, but without debugging this function. %b \n" "${WHITE}" "${NC}";
    run_level_test "NoDeBug";
    local traced; traced="$(trace_calls)";
    printf "%b%b  Stack: %s %b\n" "${YELLOW}" "⚠️" "${traced}" "${NC}";
    #
    log_msg "Pass" "Passed test";
    show_footer "${GRAY}";
    return 0; # Pass
}

#**************************************************************************************************
# @brief      play_wav(). aplay, play, ffplay, wslview, xdg-open, open
#             install_media_support
# @parm       wav2play: Full path to wav file.
# @return     0=No Error, >=1=Errors
#**************************************************************************************************
play_wav()
{
    local wav2play="${1:-}";
    local -i found_tool=0;

    if [[ -z "${wav2play}" ]]; then log_msg "Error" "[${FUNCNAME[0]}] Missing argument: wav2play"; return 1; fi
    if [[ ! -f "${wav2play}" ]]; then log_msg "Error" "[${FUNCNAME[0]}] File not found: ${wav2play}"; return 1; fi
    if [[ ! -r "${wav2play}" ]]; then log_msg "Error" "[${FUNCNAME[0]}] File not readable: ${wav2play}"; return 1; fi

    # aplay (WSL/Linux)
    if command -v aplay >/dev/null 2>&1; then
        found_tool=1;
        if (( SHOW_DEFINES == 1 )); then log_msg "Pass" "[${FUNCNAME[0]}] aplay is available for audio playback."; fi
        if aplay "${wav2play}"; then return 0; fi
    fi

    # ffplay (FFmpeg)
    if command -v ffplay >/dev/null 2>&1; then
        found_tool=1;
        if (( SHOW_DEFINES == 1 )); then log_msg "Pass" "[${FUNCNAME[0]}] ffplay is available for audio playback."; fi
        if ffplay -nodisp -autoexit -loglevel error "${wav2play}" >/dev/null 2>&1; then return 0; fi
    fi

    # wslview (WSL GUI)
    if command -v wslview >/dev/null 2>&1; then
        found_tool=1;
        if (( SHOW_DEFINES == 1 )); then log_msg "Pass" "[${FUNCNAME[0]}] wslview is available for audio playback."; fi
        if wslview "${wav2play}"; then return 0; fi
    fi

    # xdg-open (Linux GUI)
    if command -v xdg-open >/dev/null 2>&1; then
        found_tool=1;
        if (( SHOW_DEFINES == 1 )); then log_msg "Pass" "[${FUNCNAME[0]}] xdg-open is available for audio playback."; fi
        if xdg-open "${wav2play}"; then return 0; fi
    fi

    # play (sox, macOS/Linux)
    if command -v play >/dev/null 2>&1; then
        found_tool=1;
        if (( SHOW_DEFINES == 1 )); then log_msg "Pass" "[${FUNCNAME[0]}] play (sox) is available for audio playback."; fi
        if play "${wav2play}"; then return 0; fi
    fi

    # open (macOS)
    if command -v open >/dev/null 2>&1; then
        found_tool=1;
        if (( SHOW_DEFINES == 1 )); then log_msg "Pass" "[${FUNCNAME[0]}] open is available for audio playback."; fi
        if open "${wav2play}"; then return 0; fi
    fi

    if (( found_tool == 0 )); then
        log_msg "Error" "[${FUNCNAME[0]}] No audio playback tool found (aplay/play/ffplay/wslview/xdg-open/open).";
        return 1;
    fi

    log_msg "Error" "[${FUNCNAME[0]}] Audio playback failed using all available tools: ${wav2play}";
    return 1;
}

#**************************************************************************************************

log_msg "Info" "Loaded: library.sh version (${SCRIPT_VERSION_LIBRARY}) ${SCRIPT_DATE_LIBRARY}";

#**************************************************************************************************
# @footer library.sh
#**************************************************************************************************
