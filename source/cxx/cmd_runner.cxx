////////////////////////////////////////////////////////////////////////////////////////////////////
/// C++ source file: cmd_runner.cxx                                                              ///
////////////////////////////////////////////////////////////////////////////////////////////////////

// Include the primary header.
#include "cmd_runner.hxx"

// Include STL headers.
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <format>

// Include POSIX headers.
#include <fcntl.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <unistd.h>

// When building with gcov (--coverage), flush coverage data from the child process before
// execvp() replaces the process image or _exit() discards it. The GCOV_BUILD macro is defined
// in the test Makefile's COVOPTS so this block is absent in normal production builds.
#ifdef GCOV_BUILD
    extern "C" void __gcov_dump(void);
    extern "C" void __gcov_reset(void);
    #define GCOV_DUMP()  __gcov_dump()
    #define GCOV_RESET() __gcov_reset()
#else
    #define GCOV_DUMP()  do {} while (0)
    #define GCOV_RESET() do {} while (0)
#endif

// Include the headers of custom modules.
#include "error.hxx"
#include "pipe.hxx"
#include "tokenizers.hxx"
#include "utils.hxx"

////////////////////////////////////////////////////////////////////////////////////////////////////
// File-local functions
////////////////////////////////////////////////////////////////////////////////////////////////////

// Unnamed namespace for making classes and functions file-local.
namespace
{
    // Practical safety limits against endless outputs.
    constexpr SizeType MAX_OUTPUT_SIZE = 1024 * 1024;  // stdout: 1 MiB
    constexpr SizeType MAX_ERROR_SIZE  =   64 * 1024;  // stderr: 64 KiB

    // Timeouts for the child process.
    constexpr auto TIMEOUT_TOTAL = std::chrono::seconds(2);         // Whole command.
    constexpr auto TIMEOUT_GRACE = std::chrono::milliseconds(200);  // After the child exits.

    struct Stream
    // A pipe stream being read by the parent process.
    {
        int      fd;
        String*  buf;
        SizeType cap;
        bool     eof;
    };

    void drain(Stream& s)
    // Read as much as possible. Data beyond the cap is discarded, but reading continues
    // so that the child never blocks on a full pipe.
    //
    // [Args]
    //   s (Stream&): [IN/OUT] Stream to be drained.
    //
    {   // {{{

        char buffer[4096];

        while (not s.eof)
        {
            // Read data from the pipe.
            const SignedSizeType n = read(s.fd, buffer, sizeof(buffer));

            // Data available: append it to the buffer if there is space.
            if (n > 0)
            {
                if (s.buf->size() < s.cap)
                    s.buf->append(buffer, min(static_cast<SizeType>(n), s.cap - s.buf->size()));
                continue;
            }

            // EOF: the child has closed the pipe.
            if (n == 0) { s.eof = true; break; }

            // EINTR: Interrupted by a signal, try again.
            if (errno == EINTR) { continue; }

            // EAGAIN/EWOULDBLOCK: No more data available for now, stop reading.
            if ((errno == EAGAIN) or (errno == EWOULDBLOCK)) { break; }

            // Unrecoverable error: report it and stop reading.
            print_error("Warning", std::format("Failed to read from pipe: {} (errno: {})", std::strerror(errno), errno));
            s.eof = true;
            break;
        }

    }   // }}}

    Vector<char*> make_argv(const Vector<String>& cmd_tokens)
    // Build the argument vector for execvp(). The "const_cast" is safe because POSIX
    // guarantees that execvp() never modifies its arguments.
    //
    // [Args]
    //   cmd_tokens (const Vector<String>&): [IN] Command tokens.
    //
    // [Returns]
    //   (Vector<char*>): Argument vector for execvp(), null-terminated.
    //
    {   // {{{

        // Reserve space for the argument vector, including the null terminator.
        Vector<char*> argv;
        argv.reserve(cmd_tokens.size() + 1);

        for (const String& token : cmd_tokens)
            argv.emplace_back(const_cast<char*>(token.c_str()));
        argv.emplace_back(nullptr);

        return argv;

    }   // }}}

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Child-side helper functions
    ////////////////////////////////////////////////////////////////////////////////////////////////

    void write_stderr(const char* msg) noexcept
    // Write a message to stderr, ignoring errors.
    // This is used in the child process after fork() to report fatal errors before exit.
    //
    // [Args]
    //   msg (const char*): [IN] Null-terminated message to write.
    //
    {   // {{{

        [[maybe_unused]] const SignedSizeType r = write(STDERR_FILENO, msg, std::strlen(msg));

    }   // }}}

    void child_fail(void) noexcept
    // Report a fatal error and terminate the child process.
    // This is used in the child process after fork() to report fatal errors before exit.
    //
    {   // {{{

        // Flush coverage data before _exit() discards it.
        GCOV_DUMP();
        _exit(EXIT_FAILURE);

    }   // }}}

    void redirect(int fd, int target) noexcept
    // Duplicate "fd" onto "target", or terminate the child on failure.
    //
    // [Args]
    //   fd     (int): [IN] File descriptor to be duplicated.
    //   target (int): [IN] Target file descriptor to be replaced.
    //
    {   // {{{

        if (fd < 0) child_fail();

        // dup2(fd, fd) is a no-op and does NOT clear FD_CLOEXEC, so clear it explicitly.
        if (fd == target)
        {
            if (fcntl(fd, F_SETFD, 0) < 0)
                child_fail();
            return;
        }

        if (dup2(fd, target) < 0)
            child_fail();

    }   // }}}

    void redirect_devnull(int flags, int target) noexcept
    // Redirect "target" to /dev/null, or terminate the child on failure.
    //
    // [Args]
    //   flags  (int): [IN] Flags for open() (O_RDONLY or O_WRONLY).
    //   target (int): [IN] Target file descriptor to be replaced.
    //
    {   // {{{

        // Open /dev/null with O_CLOEXEC to avoid leaking it to grandchildren.
        const int fd = open("/dev/null", flags | O_CLOEXEC);

        // Redirect the target file descriptor to /dev/null.
        redirect(fd, target);

        // Close the original fd if it is not the same as the target.
        if (fd != target)
            close(fd);

    }   // }}}

    void exec_child(char* const argv[], bool capture, int out_fd, int err_fd) noexcept
    // Set up the child process and run the command.
    //
    // [Args]
    //   argv    (char* const[]): [IN] Null-terminated argument vector.
    //   capture (bool)         : [IN] True in RUN_COMMAND_GETOUT mode.
    //   out_fd  (int)          : [IN] Write end of the stdout pipe.
    //   err_fd  (int)          : [IN] Write end of the stderr pipe, or -1 to discard stderr.
    //
    {   // {{{

        // Reset gcov counters so only child-process lines are tracked in this dump.
        GCOV_RESET();

        if (capture)
        {
            setpgid(0, 0);

            // Detach stdin from the terminal, and redirect stdout/stderr.
            redirect_devnull(O_RDONLY, STDIN_FILENO);
            redirect(out_fd, STDOUT_FILENO);
            if (err_fd >= 0) { redirect(err_fd, STDERR_FILENO);           }
            else             { redirect_devnull(O_WRONLY, STDERR_FILENO); }
        }

        // The parent ignores SIGINT (see main_redalien), but the child should receive it.
        if (signal(SIGINT, SIG_DFL) == SIG_ERR)
        {
            write_stderr("RedAlien: failed to reset SIGINT in child\n");
            child_fail();
        }

        execvp(argv[0], argv);

        // Reached only when execvp() failed.
        write_stderr("RedAlien: execvp failed: ");
        write_stderr(argv[0]);
        write_stderr("\n");
        child_fail();

    }   // }}}

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Parent-side helper functions
    ////////////////////////////////////////////////////////////////////////////////////////////////

    void wait_child(pid_t pid) noexcept
    // Block until the child terminates, and reap it.
    //
    // [Args]
    //   pid (pid_t): [IN] PID of the child process.
    //
    {   // {{{

        int32_t status;
        while (waitpid(pid, &status, 0) == -1)
        {
            if (errno != EINTR)
            {
                print_error("Warning", std::format("waitpid failed: {}", std::strerror(errno)));
                return;
            }
        }

    }   // }}}

    bool has_exited(pid_t pid) noexcept
    // Check whether the child has exited, WITHOUT reaping it. Keeping the child as
    // a zombie guarantees that its PID (= PGID) is not reused until the final group kill.
    //
    // [Args]
    //   pid (pid_t): [IN] PID of the child process.
    //
    // [Returns]
    //   (bool): True if the child has exited, false if it is still running.
    //
    {   // {{{

        siginfo_t info;

        // Use waitid() with WNOWAIT to check the child status without reaping it.
        const int ret = waitid(P_PID, pid, &info, WEXITED | WNOHANG | WNOWAIT);

        // The child has exited: check if the PID matches.
        if (ret == 0) return (info.si_pid == pid);

        // EINTR: Interrupted by a signal, try again later.
        if (errno == EINTR) return false;

        // Unrecoverable error: report it and return true to avoid waiting forever.
        print_error("Warning", std::format("waitid failed: {} (errno: {})", std::strerror(errno), errno));
        return true;

    }   // }}}

    bool wait_readable(const Stream& s1, const Stream& s2) noexcept
    // Wait until either stream is readable, or 10 ms passes.
    //
    // [Args]
    //   s1 (const Stream&): [IN] First stream to wait for.
    //   s2 (const Stream&): [IN] Second stream to wait for.
    //
    // [Returns]
    //   (bool): False if select() failed with an unrecoverable error.
    //
    {   // {{{

        // Set up the read set for select().
        fd_set rfds;
        FD_ZERO(&rfds);

        // Add the file descriptors of the streams that are not at EOF to the read set.
        int max_fd = -1;
        for (const Stream* s : {&s1, &s2})
        {
            if (not s->eof)
            {
                FD_SET(s->fd, &rfds);
                max_fd = max(max_fd, s->fd);
            }
        }

        // Wait for either stream to become readable, or 10 ms to pass.
        struct timeval tv = {0, 10000};
        const int ret = select(max_fd + 1, &rfds, nullptr, nullptr, &tv);
        if ((ret < 0) and (errno != EINTR))
        {
            print_error("Warning", std::format("select failed: {} (errno: {})", std::strerror(errno), errno));
            return false;
        }

        return true;

    }   // }}}

    void collect_output(pid_t pid, Stream& s_out, Stream& s_err)
    // Read stdout/stderr of the child until both pipes reach EOF and the child exits,
    // while enforcing the total timeout and the grace period after the child exits.
    //
    // [Args]
    //   pid   (pid_t)   : [IN]     PID of the child process.
    //   s_out (Stream&) : [IN/OUT] Stream for stdout.
    //   s_err (Stream&) : [IN/OUT] Stream for stderr.
    //
    {   // {{{

        // Set the deadline for the whole command.
        std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + TIMEOUT_TOTAL;

        bool child_done = false;
        bool killed     = false;
        bool timed_out  = false;

        while (true)
        {
            // Read as much as possible from both streams.
            drain(s_out);
            drain(s_err);

            // Too much output: ask the whole process group to terminate.
            if ((s_out.buf->size() >= s_out.cap) and (not child_done) and (not killed))
            { kill(-pid, SIGTERM); killed = true; }

            // Once the child exits, wait only briefly for grandchildren holding the pipe.
            if ((not child_done) and has_exited(pid))
            {
                // The child has exited, but the pipes may still have data to read.
                child_done = true;

                // If the child exited before the deadline, shorten the deadline to the grace period.
                deadline = std::min(deadline, std::chrono::steady_clock::now() + TIMEOUT_GRACE);
            }

            // Normal completion.
            if (child_done and s_out.eof and s_err.eof) return;

            // Deadline: kill the whole group (SIGKILL cannot be ignored).
            if ((not timed_out) and (std::chrono::steady_clock::now() >= deadline))
            { kill(-pid, SIGKILL); timed_out = true; }

            // The child has exited after the deadline: stop waiting for EOF.
            if (timed_out and child_done) { drain(s_out); drain(s_err); return; }

            // Wait until either stream is readable, or 10 ms passes. This avoids busy-waiting.
            if (not wait_readable(s_out, s_err)) return;
        }

    }   // }}}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Public functions
////////////////////////////////////////////////////////////////////////////////////////////////////

String run_command(StringView command, RunCommandOption option, String* err_out)
{   // {{{

    // Tokenize the command string and run the command.
    Vector<String> cmd_tokens;
    for (const StringView token : tokenize(command, TOKENIZE_DEQUOTE))
        cmd_tokens.emplace_back(token);

    return run_command(cmd_tokens, option, err_out);

}   // }}}

String run_command(const Vector<String>& cmd_tokens, RunCommandOption option, String* err_out)
{   // {{{

    // Clear the error output first, so that stale contents never remain.
    if (err_out != nullptr) err_out->clear();
    if (cmd_tokens.empty()) return "";

    // Parse the options.
    const bool no_strip   = (option & RUN_COMMAND_NO_STRIP) != 0;
    const bool get_output = (option & RUN_COMMAND_GETOUT  ) != 0;
    const bool get_error  = get_output and (err_out != nullptr);

    // Prepare everything that allocates memory BEFORE fork().
    const Vector<char*> argv = make_argv(cmd_tokens);
    Pipe pipe_out, pipe_err;
    if (get_output and (not pipe_out.open())) return "";
    if (get_error  and (not pipe_err.open())) return "";

    // Fork and run the command in the child.
    const pid_t pid = fork();
    if (pid == -1)
    {
        print_error("Error", std::format("Failed to fork: {} (errno: {})", std::strerror(errno), errno));
        return "";
    }
    if (pid == 0)
        exec_child(argv.data(), get_output, pipe_out.fd_w(), pipe_err.fd_w());

    // Plain mode: just wait for the child.
    if (not get_output) { wait_child(pid); return ""; }

    // Capture mode: also set the process group in the parent to avoid a race with the child.
    setpgid(pid, pid);
    pipe_out.close_w();
    pipe_err.close_w();
    pipe_out.set_read_nonblock();
    pipe_err.set_read_nonblock();

    // Read the outputs.
    String output, error;
    Stream s_out{pipe_out.fd_r(), &output, MAX_OUTPUT_SIZE, false};
    Stream s_err{pipe_err.fd_r(), &error,  MAX_ERROR_SIZE,  not get_error};
    collect_output(pid, s_out, s_err);

    // Kill surviving grandchildren (safe: the child is still an unreaped zombie), then reap.
    kill(-pid, SIGKILL);
    wait_child(pid);

    // Return the outputs, optionally stripping leading and trailing whitespace.
    const auto finish = [no_strip](const String& s) { return no_strip ? s : String(strip(s)); };
    if (err_out != nullptr) *err_out = finish(error);
    return finish(output);

}   // }}}

String run_command(const Vector<StringView>& cmd_tokens, RunCommandOption option, String* err_out)
{   // {{{

    // The "StringView" is not compatible with "execvp()", because "execvp()" requires
    // null-terminated strings, but StringView does not guarantee null-termination. Therefore
    // convert the command tokens to String before running the command.

    Vector<String> cmd_tokens_str;
    for (const StringView token_view : cmd_tokens)
        cmd_tokens_str.emplace_back(token_view);

    return run_command(cmd_tokens_str, option, err_out);

}   // }}}

// vim: expandtab tabstop=4 shiftwidth=4 fdm=marker
