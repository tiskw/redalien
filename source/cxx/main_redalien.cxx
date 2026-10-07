////////////////////////////////////////////////////////////////////////////////////////////////////
/// C++ source file: main_redalien.cxx                                                           ///
////////////////////////////////////////////////////////////////////////////////////////////////////

// Include the headers of custom modules.
#include "main_redalien.hxx"

// Include STL headers.
#include <fstream>
#include <iostream>

// Include POSIX headers.
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>

// Include the header of the cxxopts library.
#include <cxxopts.hpp>

// Include the headers of custom modules.
#include "cmd_runner.hxx"
#include "config.hxx"
#include "dtypes.hxx"
#include "error.hxx"
#include "gen_path_cache.hxx"
#include "path_x.hxx"
#include "read_cmd.hxx"
#include "string_utils.hxx"
#include "tokenizers.hxx"
#include "utils.hxx"

////////////////////////////////////////////////////////////////////////////////////////////////////
// File-local functions
////////////////////////////////////////////////////////////////////////////////////////////////////

// Unnamed namespace for making classes and functions file-local.
namespace
{
    String get_git_branch_info(void)
    // Get git branch and status information and return as a colored string.
    // 
    // [Returns]
    //   (String): Colored string of git information.
    //
    {   // {{{

        // Returns empty string if not a Git directory.
        if (not stdfs::exists(".git"))
            return "";

        // Get the branch name and its status at the same time.
        // NOTE: the following is an example of the command output:
        //
        // $ git status --porcelain=v2 --branch
        // # branch.oid 53bd39614a7c66c6a3816a1c5c1e528db4c35c52
        // # branch.head alpha
        // # branch.upstream origin/alpha
        // # branch.ab +1 -0
        // 1 .M N... 100644 100644 100644 ca6703623a3e5897f3b183d5a998d6791bd3bd15 ca6703623a3e5897f3b183d5a998d6791bd3bd15 source/cxx/main_redalien.cxx
        //
        const String git_status = run_command("git status --porcelain=v2 --branch", RUN_COMMAND_GETOUT);

        // Initialize the git branch name and the changed flag.
        String branch     = "???";
        bool   is_changed = false;
    
        for (const StringView sv : split(git_status, "\n"))
        {
            // Get the branch name.
            if (sv.starts_with("# branch.head "))
                branch = sv.substr(14);

            // Set the changed flag if non-branch line is dumped.
            else if (not sv.starts_with("# "))
                is_changed = true;
        }

        // Colorize as yellow if the git status is "changed".
        if (!branch.empty() and is_changed)
            return "\x1B[38;2;235;193;111m" + branch + "!\x1B[m";

        // Colorize as green if the git status is "unchanged".
        if (!branch.empty())
            return "\x1B[38;2;181;189;104m" + branch +  "\x1B[m";

        return branch;

    }   // }}}

    bool is_valid_outdir(const Path& path)
    // Check if the given path is a valid output directory.
    //
    // [Args]
    //   path (const Path&): [IN] Path to be checked.
    //
    // [Returns]
    //   (bool): True if the path is a valid output directory, otherwise false.
    //
    {   // {{{

        struct stat st{};
        return (lstat(path.c_str(), &st) == 0) and S_ISDIR(st.st_mode)
            and (st.st_uid == ::getuid()) and ((st.st_mode & 077) == 0);

    }   // }}}

    Deque<String> read_history(StringView path_hist, uint16_t max_hist_size)
    // Read history file and return history entries.
    //
    // [Args]
    //   path_hist     (StringView): [IN] Path to history file.
    //   max_hist_size (uint16_t)  : [IN] Maximum number of history entries to be stored in the output queue.
    //
    // [Returns]
    //   (Deque<String>): A collection of history lines.
    //
    {   // {{{

        // Initialize the output queue.
        Deque<String> queue;

        // Open the history file.
        std::ifstream ifp(expand_tilde(path_hist));
        if (not ifp.is_open())
            return queue;

        // Create temporary string data.
        String line;

        // Read the history file.
        while (getline(ifp, line))
        {

            // Remove one string from the front if the queue size is too large.
            if (queue.size() == max_hist_size)
                queue.pop_front();

            // Strip the line.
            line = strip(line);

            // Append if the line is not empty.
            if (line.size() > 0)
                queue.emplace_back(line);
        }

        return queue;

    }   // }}}

    Tuple<String, String> run_keybind(const ReadCmdOut& rc_out, const StringMap& keybinds, const Path& path_plugin_out)
    // Run the given keybind.
    //
    // [Args]
    //   rc_out        (const ReadCmdOut&): [IN] Output of "readcmd" function.
    //   keybinds      (const StringMap&) : [IN] Map of keybinds in the config file.
    //   output_plugin (const String&)    : [IN] Path to the plugin output file.
    //
    // [Returns]
    //   (Tuple<String, String>): Left and right hand side of the editing buffer after keybind.
    //
    {   // {{{

        // If the given key is not registered, do nothing.
        if (not keybinds.contains(rc_out.stop))
            return {rc_out.lhs, rc_out.rhs};

        // Create a map for placeholders replacement.
        const StringMap extra = {
            {"{lhs}",           rc_out.lhs},
            {"{rhs}",           rc_out.rhs},
            {"{output_plugin}", expand_tilde(path_plugin_out.string())},
        };

        // Tokenize the command string with placeholder replacement.
        Vector<String> cmd_tokens;
        for (const String& token : tokenize_with_placeholder_replacement(keybinds.at(rc_out.stop), extra, TOKENIZE_DEQUOTE))
            cmd_tokens.emplace_back(token);

        // Run the tokenized command.
        run_command(cmd_tokens);

        // Read the output of the command from the command output file.
        std::ifstream ifs(path_plugin_out);
        if (not ifs)
        {
            std::cerr << "Failed to open file: " << path_plugin_out << std::endl;
            return {rc_out.lhs, rc_out.rhs};
        }

        // Get the left and right hand side of the editing buffer from the command output file.
        String lhs_new, rhs_new;
        std::getline(ifs, lhs_new);
        std::getline(ifs, rhs_new);

        return {lhs_new, rhs_new};

    }   // }}}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Main function
////////////////////////////////////////////////////////////////////////////////////////////////////

int32_t main_redalien(int32_t argc, char* argv[], const char* input_ptr)
{   // {{{

    if (signal(SIGINT, SIG_IGN) == SIG_ERR)
        return print_error("Error", "Failed to register signal handler");

    // Parse command line arguments.
    cxxopts::Options options(SOFTWARE_NAME, SOFTWARE_DESC);
    options.add_options()
        ("c,config",    "Path to config file.",           cxxopts::value<String>())
        ("e,editor",    "Editor mode ('emacs' or 'vi').", cxxopts::value<String>())
        ("i,input",     "User input string.",             cxxopts::value<String>())
        ("o,outdir",    "Path to output directory.",      cxxopts::value<String>())
        ("g,gen-cache", "Run other task and exit.")
        ("h,help",      "Show this help message and exit.")
        ("v,version",   "Show version information and exit.");

    // Parse the arguments and print help message if -h/--help is given.
    const auto args = options.parse(argc, argv);
    if (args.count("help"))
    {
        std::cout << "Usage:"                                                     << '\n';
        std::cout << "    redalien [OPTION...]"                                   << '\n';
        std::cout << ""                                                           << '\n';
        std::cout << "Lightweight alternative to GNU Readline for RedAlien."      << '\n';
        std::cout << ""                                                           << '\n';
        std::cout << "Behavioral options:"                                        << '\n';
        std::cout << "    -c, --config PATH   Path to config file."               << '\n';
        std::cout << "    -e, --editor STR    Editor mode ('emacs' or 'vi')."     << '\n';
        std::cout << "    -g, --gen-cache     Generate cache files and exit."     << '\n';
        std::cout << "    -o, --outdir PATH   Path to output directory."          << '\n';
        std::cout << ""                                                           << '\n';
        std::cout << "Other options:"                                             << '\n';
        std::cout << "    -h, --help          Show this help message and exit."   << '\n';
        std::cout << "    -v, --version       Show version information and exit." << '\n';
        return EXIT_SUCCESS;
    }

    // Run other task and exit if -r/--run is specified.
    if (args.count("gen-cache"))
        return generate_path_commands_cache();

    // The argument --outdir is a mandatory option for this program, threrfore if --outdir is not
    // specified or invalid, print an error message and exit. Note that we include a brief sleep
    // at the end. This is because this program is designed to be called repeatedly, and the pause
    // prevents the CPU from becoming overburdened by rapid, repetitive execution.
    const Path outdir = args.count("outdir") ? Path(args["outdir"].as<String>()) : Path("");
    if (not is_valid_outdir(outdir))
    {
        print_error("Error", "The mandatory option --outdir is not provided or invalid.");

        // Sleep for 200 milliseconds before checking again.
        struct timespec ts;
        ts.tv_sec  = 0;
        ts.tv_nsec = 200 * 1000 * 1000;
        nanosleep(&ts, nullptr);

        return EXIT_FAILURE;
    }

    // Load the config values.
    const RedAlienConfig cfg = load_config(args.count("config") ? args["config"].as<String>() : "");

    // Read the history file.
    Deque<String> hists = read_history(cfg.path_history, cfg.max_hist_size);

    // Get the editor mode.
    const String editor_name = args.count("editor") ? args["editor"].as<String>() : "emacs";

    // Get the path to the plugin output file.
    const Path path_plugin_out = outdir / "plugin.out";

    // Convert the input_ptr to a String.
    const String input_str = (input_ptr != nullptr) ? String(input_ptr) : String("");

    // Initialize text buffer (priority: command line argument > input_ptr).
    ReadCmdOut rc_out = {"", "", "", args.count("input") ? args["input"].as<String>() : input_str};

    // Start user editing loop.
    while (true)
    {
        // Get user input.
        rc_out = readcmd(rc_out.lhs, rc_out.rhs, hists, editor_name, outdir, rc_out.input, cfg);

        // Exit from the while loop if the user editing stopped without stop key.
        if (rc_out.stop.size() == 0) break;

        // Otherwise, run keybind command of the stop key, and continue the loop.
        std::tie(rc_out.lhs, rc_out.rhs) = run_keybind(rc_out, cfg.keybinds, path_plugin_out);
    }

    // Compute user input string.
    String user_input;
    user_input += rc_out.lhs;
    user_input += rc_out.rhs;

    // Print timestamp, a whitespace and the colorized user input.
    std::cout << cfg.datetime_pre << get_time(std::time(nullptr), "%Y/%m/%d %H:%M:%S") << cfg.datetime_post;
    std::cout << ' ' << colorize(user_input);

    // Write the user input to the output file if specified.
    if (args.count("outdir"))
    {
        // Get the path to the output file.
        const Path path_output = outdir / "redalien.out";

        // Open the output file.
        std::ofstream ofs(path_output);
        if (not ofs.is_open())
        {
            std::cerr << "Failed to open file: " << path_output << std::endl;
            return EXIT_FAILURE;
        }

        // Write the user input to the output file.
        ofs << user_input << '\n';

        // Close the file.
        ofs.close();
    }

    return EXIT_SUCCESS;

}   // }}}

// vim: expandtab tabstop=4 shiftwidth=4 fdm=marker
