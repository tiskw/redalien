////////////////////////////////////////////////////////////////////////////////////////////////////
/// C++ source file: config.cxx                                                                  ///
////////////////////////////////////////////////////////////////////////////////////////////////////

// Include the primary header.
#include "config.hxx"

// Include STL headers.
#include <iostream>

// Include POSIX headers.
#include <unistd.h>

// Include the header of the toml++ library.
#define TOML_EXCEPTIONS 0
#include <toml.hpp>

// Include the headers of custom modules.
#include "error.hxx"
#include "utils.hxx"

////////////////////////////////////////////////////////////////////////////////////////////////////
// File-local static functions
////////////////////////////////////////////////////////////////////////////////////////////////////

// Unnamed namespace for making classes and functions file-local.
namespace
{
    void show_error_msg_undefined_entry(const toml::key& section, const toml::key& value)
    // Show error message and quit this software.
    //
    // [Args]
    //   section (const toml::key&): [IN] Section name.
    //   value   (const toml::key&): [IN] Value name.
    //
    {   // {{{

        // Prepare error message.
        std::stringstream ss;
        ss << "Undefined entry name: '" << value << "' in section '" << section << "'";

        // Print error message and exit the function.
        print_error("Error", ss.str());

    }   // }}}

    void show_error_msg_invalid_completion_entry(const toml::node& entry)
    // Show error message and quit this software.
    //
    // [Args]
    //   entry (const toml::node&): [IN] TOML entry.
    //
    {   // {{{

        // Prepare error message.
        std::stringstream ss;
        ss << "Invalid completion entry: " << entry.as_array();

        // Print error message and exit the function.
        print_error("Error", ss.str());

    }   // }}}

    Vector<String> as_string_vector(const toml::array* array_node)
    // Convert a TOML array node to a vector of strings.
    //
    {   // {{{

        Vector<String> result;

        // Read pattern strings.
        std::transform(array_node->begin(), array_node->end(), std::back_inserter(result),
                       [](const toml::node& value) { return value.value_or(""); });

        return result;

    }   // }}}

    void set_config(RedAlienConfig& cfg, const toml::table& table, const toml::key& section, const toml::key& value)
    // Read one config item to the global variable `config`.
    //
    // [Args]
    //   cfg     (RedAlienConfig&)   : [OUT] Config instance to update.
    //   table   (const toml::table&): [IN]  Top node of the config file.
    //   section (const toml::key&)  : [IN]  Section name.
    //   value   (const toml::key&)  : [IN]  Value name.
    //
    {   // {{{

        // Get target config item.
        toml::node_view node = table[section][value];

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Read the [GENERAL] section.
        ////////////////////////////////////////////////////////////////////////////////////////////

        if      ((section == "GENERAL") and (value == "area_height"   )) cfg.area_height    = node.value_or(cfg.area_height);
        else if ((section == "GENERAL") and (value == "column_padding")) cfg.column_padding = node.value_or(cfg.column_padding);
        else if ((section == "GENERAL") and (value == "path_history"  )) cfg.path_history   = node.value_or(cfg.path_history);
        else if ((section == "GENERAL") and (value == "max_hist_size" )) cfg.max_hist_size  = node.value_or(cfg.max_hist_size);
        else if ((section == "GENERAL") and (value == "datetime_pre"  )) cfg.datetime_pre   = node.value_or(cfg.datetime_pre);
        else if ((section == "GENERAL") and (value == "datetime_post" )) cfg.datetime_post  = node.value_or(cfg.datetime_post);
        else if ((section == "GENERAL") and (value == "histhint_pre"  )) cfg.histhint_pre   = node.value_or(cfg.histhint_pre);
        else if ((section == "GENERAL") and (value == "histhint_post" )) cfg.histhint_post  = node.value_or(cfg.histhint_post);
        else if ((section == "GENERAL") and (value == "hline_char"    )) cfg.hline_char     = node.value_or(cfg.hline_char);
        else if ((section == "GENERAL") and (value == "hline_color"   )) cfg.hline_color    = node.value_or(cfg.hline_color);
        else if ((section == "GENERAL")                                ) show_error_msg_undefined_entry(section, value);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Read the [PROMPT] section.
        ////////////////////////////////////////////////////////////////////////////////////////////

        else if ((section == "PROMPT") and (value == "ps0l")) cfg.ps0l = node.value_or(cfg.ps0l);
        else if ((section == "PROMPT") and (value == "ps0r")) cfg.ps0r = node.value_or(cfg.ps0r);
        else if ((section == "PROMPT") and (value == "ps1i")) cfg.ps1i = node.value_or(cfg.ps1i);
        else if ((section == "PROMPT") and (value == "ps1n")) cfg.ps1n = node.value_or(cfg.ps1n);
        else if ((section == "PROMPT") and (value == "ps2" )) cfg.ps2  = node.value_or(cfg.ps2);
        else if ((section == "PROMPT")                      ) show_error_msg_undefined_entry(section, value);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Read the [KEYBIND] section.
        ////////////////////////////////////////////////////////////////////////////////////////////

        else if (section == "KEYBIND")
        {
            cfg.keybinds[String(value.str())] = node.value_or("");
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Read the [COMPLETION] section.
        ////////////////////////////////////////////////////////////////////////////////////////////

        else if ((section == "COMPLETION") and (value == "completions") and (node.is_array()))
        {
            // Clear the default completions.
            cfg.completions.clear();

            for (const auto& entry : *node.as_array())
            {
                // Check if the entry is an array and has at least 2 items (completion pattern and completion type).
                if ((not entry.is_array()) or (entry.as_array()->size() < 3)
                 or (not entry.as_array()->at(0).is_array()) or (not entry.as_array()->at(1).is_string()) or (not entry.as_array()->at(2).is_string()))
                {
                    show_error_msg_invalid_completion_entry(entry);
                    continue;
                }

                ////////////////////////////////////////////////////////////////////////////////////
                // Read completion pattern
                ////////////////////////////////////////////////////////////////////////////////////

                // Read pattern strings.
                Vector<String> pattern = as_string_vector(entry.as_array()->at(0).as_array());

                ////////////////////////////////////////////////////////////////////////////////////
                // Read completion type
                ////////////////////////////////////////////////////////////////////////////////////

                // Get the node of the completion type.
                const char* ctype_strptr = entry.as_array()->at(1).value_or("");

                // Compute completion type.
                CompType ctype;
                if      (strcmp(ctype_strptr, "bashcomp")        == 0) ctype = CompType::BASHCOMP;
                else if (strcmp(ctype_strptr, "carapace")        == 0) ctype = CompType::CARAPACE;
                else if (strcmp(ctype_strptr, "command")         == 0) ctype = CompType::COMMAND;
                else if (strcmp(ctype_strptr, "grep")            == 0) ctype = CompType::GREP;
                else if (strcmp(ctype_strptr, "option")          == 0) ctype = CompType::OPTION;
                else if (strcmp(ctype_strptr, "path")            == 0) ctype = CompType::PATH;
                else if (strcmp(ctype_strptr, "preview")         == 0) ctype = CompType::PREVIEW;
                else if (strcmp(ctype_strptr, "shell")           == 0) ctype = CompType::SHELL;
                else if (strcmp(ctype_strptr, "subcmd")          == 0) ctype = CompType::SUBCMD;
                else if (strcmp(ctype_strptr, "subcmd+bashcomp") == 0) ctype = CompType::SC_AND_BC;
                else                                                   ctype = CompType::PATH;

                ////////////////////////////////////////////////////////////////////////////////////
                // Read optional string
                ////////////////////////////////////////////////////////////////////////////////////

                // Get the node of the optional string.
                const char* opt_strptr = entry.as_array()->at(2).value_or("");

                // Register the triplet of completion pattern, completion type, and optional string.
                cfg.completions.emplace_back(pattern, ctype, String(opt_strptr));
            }
        }
        else if (section == "COMPLETION") show_error_msg_undefined_entry(section, value);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Read the [PREVIEW] section.
        ////////////////////////////////////////////////////////////////////////////////////////////

        else if ((section == "PREVIEW") and (value == "previews") and (node.is_array()))
        {
            // Clear the default previews.
            cfg.previews.clear();

            for (const auto& entry : *node.as_array())
            {
                if ((not entry.is_array()) or (entry.as_array()->size() < 2)
                 or (not entry.as_array()->at(0).is_string()) or (not entry.as_array()->at(1).is_array()))
                {
                    show_error_msg_invalid_completion_entry(entry);
                    continue;
                }

                // Read the pattern string and command arguments.
                const char* pattern_strptr  = entry.as_array()->at(0).value_or("");
                Vector<String> command_args = as_string_vector(entry.as_array()->at(1).as_array());

                // Register the pattern string and command arguments to the previews map.
                cfg.previews.emplace(pattern_strptr, command_args);
            }
        }
        else if ((section == "PREVIEW") and (value == "preview_delim")) cfg.preview_delim = node.value_or(cfg.preview_delim);
        else if ((section == "PREVIEW") and (value == "preview_ratio")) cfg.preview_ratio = node.value_or(cfg.preview_ratio);
        else if ((section == "PREVIEW")                               ) show_error_msg_undefined_entry(section, value);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Config values for plugins
        ////////////////////////////////////////////////////////////////////////////////////////////

        else if (StringView(section).starts_with("PLUGIN_"))
            /* The section "PLUGIN_*" will be used for plugins, so ignore in redalien. */ ;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Otherwise, show error message and exit.
        ////////////////////////////////////////////////////////////////////////////////////////////

        else show_error_msg_undefined_entry(section, value);

    }   // }}}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Public functions
////////////////////////////////////////////////////////////////////////////////////////////////////

RedAlienConfig load_config(StringView path_cfg)
{   // {{{

    // Initialize config instance with default values.
    RedAlienConfig cfg;

    // Returns the default config if the specified path is not a regular file.
    if (not stdfs::is_regular_file(path_cfg))
        return cfg;

    // Read specified TOML file.
    toml::parse_result result = toml::parse_file(path_cfg);
    if (not result)
    {
        // Prepare error message.
        std::stringstream ss;
        ss << result.error().description() << " (" << result.error().source() << ")";

        // Print error message and exit the function.
        print_error("Error", ss.str());
        return cfg;
    }

    // Steal the table from the result.
    toml::table table = std::move(result).table();

    // Parse config contents
    for (const auto& node_section : table)
    {
        // If the node is a table then read the values contained in the table.
        if (node_section.second.is_table())
            for (auto node_value : *node_section.second.as_table())
                set_config(cfg, table, node_section.first, node_value.first);
    }

    // Range checks.
    if ((cfg.area_height < 2) or (256 < cfg.area_height))
    {
        print_error("Warning", "Invalid value for 'area_height' in [GENERAL] section. It must be between 2 and 256.");
        cfg.area_height = clip(cfg.area_height, static_cast<uint16_t>(2), static_cast<uint16_t>(256));
    }
    if ((cfg.preview_ratio < 0.0) or (1.0 < cfg.preview_ratio))
    {
        print_error("Warning", "Invalid value for 'preview_ratio' in [PREVIEW] section. It must be between 0.0 and 1.0");
        cfg.preview_ratio = clip(cfg.preview_ratio, 0.0f, 1.0f);
    }

    return cfg;

}   // }}}

// vim: expandtab tabstop=4 shiftwidth=4 fdm=marker
