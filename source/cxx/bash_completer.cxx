////////////////////////////////////////////////////////////////////////////////////////////////////
/// C++ source file: bash_completer.cpp                                                          ///
////////////////////////////////////////////////////////////////////////////////////////////////////

// Include the primary header.
#include "bash_completer.hxx"

// Include STL headers.
#include <cerrno>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

// Include POSIX headers.
#include <fcntl.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// Bash initialisation script sent to the child process on startup.
//   - Sources the system-wide bash-completion library.
//   - Defines the __bc_complete__ helper function used by complete().
////////////////////////////////////////////////////////////////////////////////////////////////////

static const char INIT_SCRIPT[] = R"BASH_SCRIPT(
set +e
source /usr/share/bash-completion/bash_completion 2>/dev/null || true

# Completion helper function.
# Argument: The partial command-line string typed by the user (e.g. "git sta").
# Output:   One completion candidate per line written to stdout.
function __bc_complete__ ()
{
    # Safeguard for empty input.
    (( $# >= 1 )) || return 0

    # Set the editing buffer and cursor position.
    COMP_LINE=$*
    COMP_POINT=${#COMP_LINE}

    # Set the completion words and current word index.
    COMP_WORDS=("${@}")
    COMP_CWORD=$((${#COMP_WORDS[@]} - 1))

    # Dynamically load the completion definition for the given command.
    if declare -f _completion_loader &>/dev/null; then
        _completion_loader "${COMP_WORDS[0]}" 2>/dev/null || true
    fi

    # Retrieve the completion spec and completion function.
    local comp_spec=$(complete -p "${COMP_WORDS[0]}")
    local comp_func=$(sed -n 's/.*-F \([^ ]*\).*/\1/p' <<< "${comp_spec}")

    # Reset the previous completion results.
    COMPREPLY=()

    # Run the completion function if it found.
    if [[ -n "$comp_func" ]]; then
        "$comp_func" 2>/dev/null || true
    fi

    printf '%s\n' "${COMPREPLY[@]}"
}
)BASH_SCRIPT";

////////////////////////////////////////////////////////////////////////////////////////////////////
// BashCompleter: Constructors and destructors
////////////////////////////////////////////////////////////////////////////////////////////////////

BashCompleter::BashCompleter(void)
{   // {{{

    // Create two pipes for communication with the child bash process.
    int to_bash[2], from_bash[2];
    if ((pipe2(to_bash, O_CLOEXEC) < 0) or (pipe2(from_bash, O_CLOEXEC) < 0))
        throw std::runtime_error(String("pipe2() failed: ") + strerror(errno));

    // Fork a child process to run bash.
    this->pid = fork();
    if (this->pid < 0)
    {
        // Close all the pipes if fork() failed, to avoid resource leaks.
        close(to_bash[0]); close(to_bash[1]); close(from_bash[0]); close(from_bash[1]);

        throw std::runtime_error(String("fork() failed: ") + strerror(errno));
    }

    if (this->pid == 0)
    {
        // Child process: redirect the pipes to stdin and stdout.
        dup2(to_bash[0],   STDIN_FILENO);
        dup2(from_bash[1], STDOUT_FILENO);

        // Silence stderr so that errors from completion scripts stay hidden.
        const int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) { dup2(devnull, STDERR_FILENO); close(devnull); }

        // Close all the pipes if fork() failed, to avoid resource leaks.
        close(to_bash[0]); close(to_bash[1]); close(from_bash[0]); close(from_bash[1]);

        execlp("bash", "bash", "--norc", "--noprofile", nullptr);

        // If execlp() returns, it means the exec failed.
        _exit(EXIT_FAILURE);
    }

    // Parent process: close the unused ends of both pipes.
    close(to_bash[0]); close(from_bash[1]);
    this->write_fd = to_bash[1];
    this->read_fd  = from_bash[0];

    // Send the initialisation script and wait for the sentinel to confirm
    // that bash-completion has been loaded successfully.
    send_line(INIT_SCRIPT);
    send_line(String("echo ") + String(SENTINEL));
    read_until_sentinel(15000);  // Allow up to 15 seconds for the initial load.

}   // }}}

BashCompleter::~BashCompleter(void)
{   // {{{

    // Close the pipes to the child process and reset the file descriptors.
    if (this->write_fd >= 0) { close(this->write_fd); this->write_fd = -1; }
    if (this->read_fd  >= 0) { close(this->read_fd);  this->read_fd  = -1; }

    // Wait for the child process to exit, or force-kill it if it doesn't.
    if (this->pid > 0)
    {
        // Give the child process a chance to exit gracefully.
        for (int i = 0; i < 100; ++i)
        {
            // Check if the child process has exited without blocking.
            if (waitpid(this->pid, nullptr, WNOHANG) != 0)
            { this->pid = -1; break; }

            // Sleep for 10 milliseconds before checking again.
            struct timespec ts;
            ts.tv_sec  = 0;
            ts.tv_nsec = 10 * 1000 * 1000;
            nanosleep(&ts, nullptr);
        }

        // If the child process is still running, force-kill it.
        if (this->pid > 0)
        {
            kill(this->pid, SIGKILL);
            waitpid(this->pid, nullptr, 0);
            this->pid = -1;
        }
    }

}   // }}}

////////////////////////////////////////////////////////////////////////////////////////////////////
// BashCompleter: Member functions
////////////////////////////////////////////////////////////////////////////////////////////////////

void BashCompleter::send_line(StringView line)
{   // {{{

    String cmd{line};
    cmd += '\n';
    const char* ptr       = cmd.data();
    size_t      remaining = cmd.size();
    while (remaining > 0)
    {
        const ssize_t n = write(this->write_fd, ptr, remaining);
        if (n <= 0) break;
        ptr       += n;
        remaining -= static_cast<size_t>(n);
    }

}   // }}}

String BashCompleter::read_until_sentinel(int timeout_ms)
{   // {{{

    String buf;
    char tmp[4096];
    const String sentinel_line = String(SENTINEL) + "\n";

    while (true)
    {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(this->read_fd, &fds);

        timeval tv{timeout_ms / 1000, (timeout_ms % 1000) * 1000};
        const int ret = select(this->read_fd + 1, &fds, nullptr, nullptr, &tv);
        if (ret <= 0) break;  // Timed out or encountered an error.

        const ssize_t n = read(this->read_fd, tmp, sizeof(tmp) - 1);
        if (n <= 0) break;
        tmp[n] = '\0';
        buf += tmp;

        const auto pos = buf.find(sentinel_line);
        if (pos != String::npos)
        {
            buf.resize(pos);
            break;
        }
    }

    return buf;

}   // }}}

Vector<String> BashCompleter::complete(const Vector<StringView>& tokens)
{   // {{{

    // Create a command string that is sent to the __bc_complete__ function.
    String cmd = "__bc_complete__ ";
    for (const StringView token : tokens)
        cmd += ' ' + shell_quote(token);

    // Send the user input to the bash child process and request completions.
    send_line(cmd);
    send_line(String("echo ") + String(SENTINEL));

    // Read the output from the bash child process until the sentinel is encountered.
    const String output = read_until_sentinel();

    // Split the output into lines and return them as a vector of strings.
    Vector<String> result;
    std::istringstream iss(output);
    String line;
    while (std::getline(iss, line))
        if (not line.empty())
            result.push_back(std::move(line));

    return result;

}   // }}}

////////////////////////////////////////////////////////////////////////////////////////////////////
// BashCompleter: Static functions
////////////////////////////////////////////////////////////////////////////////////////////////////

String BashCompleter::shell_quote(StringView sv)
{   // {{{

    // Wrap the string in single quotes and escape any embedded single quotes
    // using the '\'  idiom, making it safe to pass as a shell argument.
    String result = "'";
    for (const char c : sv)
    {
        if (c == '\'') { result += "'\\''"; }
        else           { result += c;       }
    }
    result += "'";
    return result;

}   // }}}

// vim: expandtab tabstop=4 shiftwidth=4 fdm=marker
