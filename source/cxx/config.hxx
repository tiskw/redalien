////////////////////////////////////////////////////////////////////////////////////////////////////
/// C++ header file: config.hxx                                                                  ///
///                                                                                              ///
/// Define a config class and functions to load config from a TOML file.                         ///
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CONFIG_HXX
#define CONFIG_HXX

// Include the headers of custom modules.
#include "dtypes.hxx"
#include "utils.hxx"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Data type declaration
////////////////////////////////////////////////////////////////////////////////////////////////////

struct RedAlienConfig
{
    ////////////////////////////////////////////////////////////////////////////
    // General settings.
    ////////////////////////////////////////////////////////////////////////////

    // Height of the drawing area.
    uint16_t area_height = 6;

    // Margin of column display in the completion list.
    uint16_t column_padding = 3;

    // Path to history file.
    String path_history = "~/.bash_history";

    // The maximum number of histories to read.
    uint16_t max_hist_size = 1000;

    // Prefix and postfix strings of the datetime string.
    String datetime_pre  = "[";
    String datetime_post = "]";

    // Prefix and postfix strings of the command history completions.
    String histhint_pre  = "";
    String histhint_post = "";

    ////////////////////////////////////////////////////////////////////////////
    // Prompt strings.
    ////////////////////////////////////////////////////////////////////////////

    // First prompt string when insert mode.
    String ps1i = "=>> ";

    // First prompt string when normal mode.
    String ps1n = "<<= ";

    // Second prompt string.
    String ps2 = "... ";

    // Additional prompt string.
    String ps_ex = "";

    ////////////////////////////////////////////////////////////////////////////
    // Keybind settings.
    ////////////////////////////////////////////////////////////////////////////

    // Key codes for completion and history completion.
    String cand_comp_key = "^I";  // Ctrl-I (= TAB key)
    String hist_comp_key = "^E";  // Ctrl-E

    // Map of input key and command.
    StringMap plugin_trigger_keys = {
        {"^F", "{path_plugin} omnipicker -m file -o {output_plugin} -l {lhs} -r {rhs}"},
        {"^U", "{path_plugin} omnipicker -m hist -o {output_plugin} -l {lhs} -r {rhs}"},
        {"^P", "{path_plugin} omnipicker -m pid  -o {output_plugin} -l {lhs} -r {rhs}"},
        {"^L", "clear"},
    };

    ////////////////////////////////////////////////////////////////////////////
    // Completion settings.
    ////////////////////////////////////////////////////////////////////////////

    // Completion patterns and their types and optional strings.
    Vector<Completion> completions = {
        {{""},               CompType::PATH,     ""},
        {{"[./~].*"},        CompType::PATH,     ""},
        {{".+",},            CompType::COMMAND,  ""},
        {{">>", "FILE", ""}, CompType::PREVIEW,  ""},
        {{">>", ".*"},       CompType::CARAPACE, ""},
    };

    ////////////////////////////////////////////////////////////////////////////
    // Preview settings.
    ////////////////////////////////////////////////////////////////////////////

    // Collection of MIME type and its preview command.
    // The {path} in the command strings will be replaced to the path to the file
    // to be previewed.
    //
    // NOTE: DO NOT USE double quote characters (") inside the command. Please
    //       use single quote (') instead.
    StringMap previews = {
        {"audio/*", "timeout 0.1s file {path}"},
        {"image/*", "timeout 0.1s file {path}"},
        {"video/*", "timeout 0.1s file {path}"},
    };

    // Delimiter of the preview window.
    String preview_delim = " │ ";

    // Width of the preview window.
    float preview_ratio = 0.45;

    ////////////////////////////////////////////////////////////////////////////
    // Color settings.
    ////////////////////////////////////////////////////////////////////////////

    StringMap colors = {
        {"red",     "\x1B[38;2;204;102;102m"},  // Red
        {"green",   "\x1B[38;2;181;189;104m"},  // Green
        {"yellow",  "\x1B[38;2;240;198;116m"},  // Yellow
        {"blue",    "\x1B[38;2;100;175;239m"},  // Blue
        {"magenta", "\x1B[38;2;178;148;187m"},  // Magenta
        {"cyan",    "\x1B[38;2;138;190;183m"},  // Cyan
        {"gray",    "\x1B[38;2;197;200;198m"},  // Gray
        {"reset",   "\x1B[0m"},                 // Reset
    };
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////////////////////////

RedAlienConfig load_config(StringView path_cfg);
// Load config TOML file.
//
// [Args]
//   path_cfg (StringView): [IN] Path to config file.
//
// [Returns]
//   (RedAlienConfig): Config instance.

#endif

// vim: expandtab tabstop=4 shiftwidth=4 fdm=marker
