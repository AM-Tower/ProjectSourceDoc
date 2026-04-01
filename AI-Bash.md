# README AI Rules
#
# Bash AI Rules:
# ****************************************************************
# Bash Style Guide (Jeffrey Standard)
#
# 1. Formatting and Style
#    - Indentation: 4 spaces per level.
#    - Braces:
#        * Allman braces
#        * Opening brace on a new line after function name.
#        * Closing brace on its own line.
#    - Variables:
#        * Always use ${var} (except in awk).
#        * Use 'local' inside functions, 'declare -gx' outside.
#    - Arrays:
#        * Use arrays and associative arrays for collections.
#    - Loops:
#        * Put 'do' on same line: while IFS= read -r x; do
#    - No blank lines inside functions; use '#' for separators.
#    - Do not use escape to wrap a line unless it is greater than 132 characters.
#    - If an if can fit on one-liner do it: if [[ -z "var" ]]; then log_msg "Error" "..."; return 1; fi
#
# 2. Comments and Documentation
#    - If blocks should have comments about if.
#    - File Header: describe purpose at top of file.
#    - Function Header: Doxygen-style @brief, @param, @return.
#    - Section Headers: group related variables/functions.
#    - Footer header: Last lines in the file will show source file name
#    - Header means it has a line above and below the Doxygen text
#    #**************************************************************************************************
#    # @footer main.sh
#    #**************************************************************************************************
#
# 3. Command Usage
#    - Quote all variable references.
#    - Check commands with: command -v <cmd>.
#    - Error handling:
#        * Use: set -euo pipefail.
#        * Add semicolon at end of every command.
#        * Never add semicolon after: if/then/else/elif/fi/for/while/until/case/do/done.
#        * All commands on same line; no '\' line breaks.
#    - Arithmetic:
#        * Use -i to declare all numbers and use if (( var == 0 )).
#        * Use var=$((var + 1)), never ((var++)), it crashes in WSL.
#    - ShellCheck‑safe: no [[ $? -eq 0 ]] — use if var="$(cmd)"; then
#
# 4. Output
#    - Use printf, not echo.
#    - Use ANSI colors.
#    - Keep output formatting consistent.
#    - printf "%b%s%b\n" "${BLUE}" "Message" "${NC}";
#    - Use wrapper log_msg [Info, Warning, Error, ...] "Message";
#    - Embedded Python code that outputs the standard screen print must use log_msg in bash.
#
# 5. Modularity and Reusability
#    - Use functions for reusable logic.
#    - Validate argument count and file/folder existence.
#    - Use trap for signal handling.
#
# 6. Structure of Scrips
#    - Shebang
#    - Header with Doxygen: @file, @brief, @author, @date, @version, @description, @param, @example
#    - @example: clear; cd ~/App && chmod +x main.sh && dos2unix -q main.sh && shellcheck main.sh && ./main.sh --help
#    - @version
#    - declare -gx SCRIPT_VERSION_MAIN="0.0.1";
#    - @date:
#    - declare -gx SCRIPT_DATE_MAIN="[2026-02-18 @ 1200]"; # YYYY-MM-DD @ HHMM (24hr)
#
# 7. Best Practices
#    - Do not alter unrelated code.
#    - Do not optimize unless asked. Do not remove functions or functionality.
#    - If reducing lines, justify no loss of functionality.
#
# 8. Tools and References
#    - Style Guide: https://google.github.io/styleguide/bashguide.html
#    - Linting: https://www.shellcheck.net/
# ****************************************************************
#

# Python Rules:
# 1. Ensure Doxygen Headers.
# Top header:
# """
# @file file.py
# @brief Description.
# """
#
# Footer header:
# ------------------------------------------------------------------
# @brief End of file.py
# ------------------------------------------------------------------
#
# class header:
# ------------------------------------------------------------------
# @class ClassName
# @brief Class Description.
# ------------------------------------------------------------------
#
# function header:
# ------------------------------------------------------------------
# @brief  Function description.
# @param  Parameters
# @returns Return value
# ------------------------------------------------------------------
#
# Do not change code that has comments to fix pylint
# All lines are ≤132 characters, imports are ordered, and all functions/classes have docstrings.
