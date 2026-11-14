////////////////////////////////////////////////////////////////////////////////////////////////////
/// C++ source file: main.cxx                                                                    ///
////////////////////////////////////////////////////////////////////////////////////////////////////

// Name of the software.
constexpr char SOFTWARE_NAME[] = "omnipicker";

// Description of the software.
constexpr char SOFTWARE_DESC[] = "A text-based user interface to pick filepaths, command history, "
                                 "process IDs, environment variables, and past tokens.";

// Version information.
constexpr char VERSION[] = "2026.08.30";

// Include the headers of STL.
#include <clocale>
#include <cstdlib>
#include <fstream>
#include <iostream>

// Include the POSIX headers.
#include <sys/ioctl.h>
#include <unistd.h>

// Include third-party headers.
#include "cxxopts.hpp"

// Include custom headers.
#include "config.hxx"
#include "dtypes.hxx"
#include "file_picker.hxx"
#include "grid_picker.hxx"
#include "line_picker.hxx"
#include "utils.hxx"

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Main functions
////////////////////////////////////////////////////////////////////////////////////////////////////

int32_t main_omnipicker(int32_t argc, const char* argv[])
// Main logic of the omnipicker plugin. Parses command-line arguments,
// runs the interactive TUI, and writes the output in RedAlien plugin format.
//
// [Args]
//   argc (int32_t)      : [IN] The number of command line arguments.
//   argv (const char*[]): [IN] The array of command line arguments.
//
// [Returns]
//   (int32_t): Exit code (0 for success, non-zero for failure).
//
{   // {{{

    // Set locale so wcwidth() and ncurses handle UTF-8 characters correctly.
    std::setlocale(LC_ALL, "");

    // Parse command line arguments.
    cxxopts::Options options("omnipicker", "omnipicker");
    options.add_options()
        ("f,file",    "Path to input/output file.", cxxopts::value<String>())
        ("m,mode",    "Mode of the picker.",        cxxopts::value<String>())
        ("c,config",  "Path to config file.",       cxxopts::value<String>()->default_value(""))
        ("i,input",   "Input string.",              cxxopts::value<String>()->default_value(""))
        ("h,help",    "Show help message and exit.")
        ("v,version", "Show version info and exit.");

    // Parse the arguments and print help message if -h/--help or -v/--version is given.
    const auto args = options.parse(argc, argv);
    if (args.count("help"))
    {
        std::cout << "Usage:"                                                                   << '\n';
        std::cout << "    " << SOFTWARE_NAME << " -f PATH -m STR [-c PATH] [-i STR]"            << '\n';
        std::cout << "    " << SOFTWARE_NAME << " (-h|--help)"                                  << '\n';
        std::cout << "    " << SOFTWARE_NAME << " (-v|--version)"                               << '\n';
        std::cout << ""                                                                         << '\n';
        std::cout << SOFTWARE_DESC                                                              << '\n';
        std::cout << ""                                                                         << '\n';
        std::cout << "Necessary arguments:"                                                     << '\n';
        std::cout << "  -f, --file PATH     Path to input/output file."                         << '\n';
        std::cout << "  -m, --mode STR      Mode of the picker (file, hist, pid, env, or val)." << '\n';
        std::cout << ""                                                                         << '\n';
        std::cout << "Optional arguments:"                                                      << '\n';
        std::cout << "  -c, --config PATH   Path to config file. [default: '']"                 << '\n';
        std::cout << "  -i, --input STR     Input string.        [default: '']"                 << '\n';
        std::cout << ""                                                                         << '\n';
        std::cout << "Other options:"                                                           << '\n';
        std::cout << "  -h, --help          Show help message and exit."                        << '\n';
        std::cout << "  -v, --version       Show version info and exit."                        << '\n';
        return EXIT_SUCCESS;
    }
    if (args.count("version"))
    {
        std::cout << VERSION << '\n';
        return EXIT_SUCCESS;
    }

    // Check if the necessary arguments are provided.
    if (not args.count("file") or not args.count("mode"))
    {
        std::cerr << "omnipicker: error: -f/--file and -m/--mode are required.\n";
        return EXIT_FAILURE;
    }

    // Terminate the program if the terminal size is too small.
    struct winsize w;
    if ((ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) or (w.ws_col < 40) or (w.ws_row < 8))
    {
        std::cerr << "omnipicker: error: terminal size is too small (minimum: 40x8)\n";
        return EXIT_FAILURE;
    }

    // Get the parsed command line arguments.
    const String path_io  = args["file"  ].as<String>();
    const String mode     = args["mode"  ].as<String>();
    const String path_cfg = args["config"].as<String>();
    const String input    = args["input" ].as<String>();

    // Read the input file and extract the lhs and rhs strings.
    String lhs, rhs;
    if (std::ifstream ifs(path_io); ifs)
    {
        std::getline(ifs, lhs);
        std::getline(ifs, rhs);
        ifs.close();
    }

    // Load the configuration values.
    OmniPickerConfig cfg = get_config(path_cfg);

    // Run the interactive TUI and collect the selected path(s).
    String lhs_updated;
    switch (hash(mode))
    {
        case hash("file"): lhs_updated = run_file_picker(lhs, cfg, input); break;
        case hash("env" ): lhs_updated = run_env_picker (lhs, cfg, input); break;
        case hash("hist"): lhs_updated = run_hst_picker (lhs, cfg, input); break;
        case hash("pid" ): lhs_updated = run_pid_picker (lhs, cfg, input); break;
        case hash("val" ): lhs_updated = run_grid_picker(lhs, cfg, input); break;

        default:
            std::cerr << SOFTWARE_NAME << ": error: invalid mode: " << mode << '\n';
            return EXIT_FAILURE;
    }

    // Get the result string.
    // If the picker is canceled (i.e. the selected string is empty), just return "lhs" and "rhs".
    const String output = lhs_updated + "\n" + rhs + "\n";

    // Open the output file for writing.
    std::ofstream ofp(path_io);
    if (not ofp.is_open())
    {
        std::cerr << SOFTWARE_NAME << ": error: cannot open output file: " << path_io << '\n';
        std::exit(EXIT_FAILURE);
    }

    // Write the output to the file and close it.
    ofp << output;
    ofp.close();

    return EXIT_SUCCESS;

}   // }}}

int main(int argc, const char* argv[])
// The entry point of the omnipicker plugin.
//
// [Args]
//   argc (int)          : The number of command line arguments.
//   argv (const char*[]): The array of command line arguments.
//
// [Returns]
//   (int): Exit code (0 for success, non-zero for failure).
//
{   // {{{

    try
    {
        return main_omnipicker(static_cast<int32_t>(argc), argv);
    }
    catch (const std::exception& e)
    {
        std::cerr << "omnipicker: \033[33mFatal\033[0m (main.cxx)\n";
        std::cerr << "-> Unhandled exception: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    catch (...)
    {
        std::cerr << "omnipicker: \033[33mFatal\033[0m (main.cxx)\n";
        std::cerr << "-> Unknown non-std exception thrown\n";
        return EXIT_FAILURE;
    }

}   // }}}

// vim: expandtab tabstop=4 shiftwidth=4 fdm=marker
