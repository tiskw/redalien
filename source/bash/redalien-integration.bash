####################################################################################################
# Takeover readline on Bash via DSR (Device Status Report) auto-trigger mechanism.
#
# Usage:
#     Source this file in your interactive shell or in your bashrc.
####################################################################################################

__REDALIEN_BIN_DIR__=$(dirname $(readlink -f "${BASH_SOURCE[0]}"))
# The bin directory of the install directory below.

__REDALIEN_INSTALL_DIR__=$(dirname "${__REDALIEN_BIN_DIR__}")
# The directory where this script and redalien binary are installed.

__REDALIEN_BINARY__="${__REDALIEN_INSTALL_DIR__}/bin/redalien"
# The path to the redalien binary.

__REDALIEN_TRIGGER__='\C-x\C-r\C-x'
# Trigger key sequence for readline to invoke redalien.

# Do nothing if not interactive shell.
[[ $- == *i* ]] || return 0

# Do nothing if already loaded.
[[ -n "${REDALIEN_LOADED:-}" ]] && return 0
REDALIEN_LOADED=1

#---------------------------------------------------------------------------------------------------
# Checker functions
#---------------------------------------------------------------------------------------------------

function __redalien_probe_dsr__ ()
# Check if the terminal supports DSR trigger.
# Send ESC[5n to the terminal and check if ESC[0n is returned.
#
# [Returns]
#    0: DSR available
#    1: not available or timeout
#
{   # {{{

    # If the terminal is dumb or not a tty, return 1 (not supported).
    [[ "${TERM:-dumb}" == "dumb" ]] && return 1
    [[ -t 0 && -t 1 ]]              || return 1
    [[ -r /dev/tty ]]               || return 1

    # Get the current terminal settings to restore later.
    local old_stty response rc
    old_stty=$(stty -g </dev/tty 2>/dev/null) || return 1

    # Change the terminal to raw mode and disable echo to reliably read the response.
    stty raw -echo </dev/tty 2>/dev/null || return 1

    # Send DSR request to the terminal.
    # The terminal should respond with ESC[0n if it supports DSR.
    printf '\e[5n' >/dev/tty

    # Read the terminal output with a timeout.
    # If the terminal responds with ESC[0n, it supports DSR.
    IFS= read -rs -t "${REDALIEN_DSR_TIMEOUT:-0.3}" -d 'n' response </dev/tty
    rc=${?}

    # Restore the original terminal settings.
    stty "${old_stty}" </dev/tty 2>/dev/null

    # If timed out (rc > 128) or unexpected response, consider it unsupported.
    (( rc > 128 )) && return 1
    [[ "${response}" == *'[0'* ]] || return 1

    return 0

}   # }}}

#---------------------------------------------------------------------------------------------------
# Select configration file to use
#---------------------------------------------------------------------------------------------------

function __redalien_select_existing_file__ ()
# Select the first existing file from the given list of files, and return its path.
#
# [Args]
#   $@: List of file paths to check.
#
# [Returns]
#   The first existing file path, or "-" if none of the files exist.
#
{   # {{{

    local file

    # Find the first existing file from the given list, print its path, and exit this function.
    for file in "${@}"; do
        if [ -f "${file}" ]; then
            echo "${file}"
            return 0
        fi
    done

    # Otherwise, print an error message and exit this function.
    echo "-"
    return 0

}   # }}}

# Config file candidates to load.
__REDALIEN_CONFIG_FILES__=(
    "${HOME}/.config/redalien/config.toml"
    "${__REDALIEN_INSTALL_DIR__}/default/config.toml"
)

# Select the bashrc and config files to load.
__REDALIEN_CONFIG_PATH__=$(__redalien_select_existing_file__ "${__REDALIEN_CONFIG_FILES__[@]}")

#---------------------------------------------------------------------------------------------------
# Custom "redalien" command
#---------------------------------------------------------------------------------------------------

function redalien ()
# Original redalien function to wrap the redalien binary.
#
# [Args]
#   $@: Arguments to pass to the redalien binary.
#
# [Returns]
#   0: Success.
#   1: Failed to run this command.
#
{   # {{{

    if [[ ${#} -eq 2 && ${1} == cache && ${2} == refresh ]]; then
        echo "RedAlien: Refreshing caches..."
        __redalien_command_cache_refresh__
    fi

}   # }}}

function __redalien_command_cache_refresh__ ()
# Refresh the command cache for redalien.
#
# [Returns]
#   0: Success.
#
{   # {{{

    # Generate a command cache file if not exists.
    local path_cmnd_info="${__REDALIEN_OUTDIR__}/cmnd_info.txt"
    if [ ! -f "${path_cmnd_info}" ]; then
        ${__REDALIEN_BINARY__} -g > "${path_cmnd_info}" 2>/dev/null
    fi

    # Generate a bash cache file if not exists.
    local path_bash_info="${__REDALIEN_OUTDIR__}/bash_info.txt"
    if [ ! -f "${path_bash_info}" ]; then
        compgen -ab          > "${path_bash_info}" 2>/dev/null
        compgen -A function >> "${path_bash_info}" 2>/dev/null
    fi

}   # }}}

#---------------------------------------------------------------------------------------------------
# Functions for DSR trigger trick
#---------------------------------------------------------------------------------------------------

__redalien_dsr_pending__=0
# A local flag to indicate that a DSR trigger is pending.
# Set to 1 when DSR is triggered, and reset to 0 after redalien is called.

function __redalien_call__ ()
# Call redalien to read a command line from the user.
#
# [Args]
#   $1 (optional): The initial input to pass to redalien (for keys trigger).
#
# [Returns]
#   0: Success.
#   1: Failed to call redalien.
#
{   # {{{

    # Ignore if no DSR trigger is pending.
    (( __redalien_dsr_pending__ )) || return 0
    __redalien_dsr_pending__=0

    # Prevent re-entry.
    # If this function is called from within the widget, do nothing.
    [[ -n "${__redalien_active__:-}" ]] && return 0
    local __redalien_active__=1

    # Create the temporary output file for redalien to write the result.
    # To prevent injection attacks, the permission of the directory is set to 700.
    local outfile="${__REDALIEN_OUTDIR__}/redalien.out"
    mkdir -p -m 700 "$(dirname "${outfile}")" 2>/dev/null

    # Inherit the editing mode from the current shell's settings.
    local editor="emacs"
    [[ ! -z "$(set -o | grep '^vi ' | grep 'on')" ]] && editor="vi"

    # Call redalien with the provided input and output file.
    if ! ${__REDALIEN_BINARY__} -c "${__REDALIEN_CONFIG_PATH__}" -e "${editor}" -o "${__REDALIEN_OUTDIR__}" -i "${1}"; then
        return 1
    fi

    # Read the result from the output file and remove any carriage return characters.
    local result=$(tr -d '\r' < ${outfile})

    # Handle special cases for the result.
    case "${result}" in
        '^D') READLINE_LINE="exit"    ;;
        '^C') READLINE_LINE=""        ;;
        *   ) READLINE_LINE="$result" ;;
    esac

    READLINE_POINT=${#READLINE_LINE}

    return 0

}   # }}}

function __redalien_dsr_kick__ ()
# Kick the DSR trigger to invoke redalien.
# This function just sends ESC[5n to the terminal.
#
# [Returns]
#   0: Success.
#
{   # {{{

    __redalien_dsr_pending__=1
    printf '\e[5n' > /dev/tty;

}   # }}}

function __redalien_enable_dsr__ ()
# Enables the DSR trigger of RedAlien.
#
# [Returns]
#   0: Success.
#
{   # {{{

    # Setup key binding: DSR -> RedAlien trigger key.
    bind "\"\e[0n\": \"${__REDALIEN_TRIGGER__}\C-m\""

    # Setup key binding: RedAlien trigger key -> redalien.
    bind -x "\"${__REDALIEN_TRIGGER__}\": __redalien_call__"

    # Store the original PROMPT_COMMAND to restore later.
    __REDALIEN_ORIG_PROMPT_COMMAND__="${PROMPT_COMMAND}"

    # Append commands to synchronize the command histories.
    PROMPT_COMMAND="${PROMPT_COMMAND}${PROMPT_COMMAND:+; }history -a; history -n"

    # Setup PROMPT_COMMAND to kick DSR key before each prompt.
    PROMPT_COMMAND="__redalien_dsr_kick__${PROMPT_COMMAND:+; $PROMPT_COMMAND}"

}   # }}}

#---------------------------------------------------------------------------------------------------
# Setup and cleanup functions for RedAlien integration
#---------------------------------------------------------------------------------------------------

function __redalien_setup__ ()
# Setup RedAlien in the current shell.
#
# [Returns]
#   0: Success.
#
{   # {{{

    # Create a output directory for RedAlien to store its output files.
    local tmpdir_rom="/tmp/redalien-$(id -u)"
    local tmpdir_ram="/dev/shm/redalien-$(id -u)"
    if   [ -d /dev/shm ] && mkdir -p ${tmpdir_ram}; then __REDALIEN_OUTDIR__="${tmpdir_ram}";
    elif [ -d /tmp     ] && mkdir -p ${tmpdir_rom}; then __REDALIEN_OUTDIR__="${tmpdir_rom}";
    else
        printf 'RedAlien: ERROR: Failed to create output directory in /tmp or /dev/shm.\n' >&2
        return 1
    fi

    # Do nothing if redalien_readcmd is not found.
    command -v "${__REDALIEN_BINARY__}" >/dev/null 2>&1 || return 0

    # Generate the command cache for redalien if not exists.
    __redalien_command_cache_refresh__

    # Do nothing if the terminal does not support DSR trigger.
    if ! __redalien_probe_dsr__; then
        printf 'RedAlien: ERROR: DSR trigger is not supported in this terminal.\n' >&2
        return 1
    fi

    # Enables the DSR trigger of RedAlien.
    __redalien_enable_dsr__

    # Store the original prompt strings to restore later.
    __REDALIEN_ORIG_PS1__="${PS1}"
    __REDALIEN_ORIG_PS2__="${PS2}"

    # Minimize the prompt string to avoid double printing of the prompt.
    PS1=''
    PS2=''

    # Set the flag to indicate that RedAlien is active.
    export REDALIEN_ACTIVE=1

}   # }}}

function __redalien_cleanup__ ()
# Cleanup RedAlien settings and restore the original readline settings.
#
# [Returns]
#   0: Success.
#
{   # {{{

    # Remove the key bindings.
    bind -r "\e[0n"               2>/dev/null
    bind -r "${__REDALIEN_RELAY__}" 2>/dev/null

    # Restore the original prompt strings.
    PS1="${__REDALIEN_ORIG_PS1__}"
    PS2="${__REDALIEN_ORIG_PS2__}"

    # Restore the original PROMPT_COMMAND.
    PROMPT_COMMAND="${__REDALIEN_ORIG_PROMPT_COMMAND__}"

    # Remove the temporary directories.
    rm -fr "${__REDALIEN_OUTDIR__}"

    # Remove unnecessary variables.
    unset REDALIEN_ACTIVE

    printf "RedAlien: cleanup done.\n" >&2

}   # }}}

#---------------------------------------------------------------------------------------------------
# Main part of this script
#---------------------------------------------------------------------------------------------------

__redalien_setup__

# vim: expandtab tabstop=4 shiftwidth=4 fdm=marker
