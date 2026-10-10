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

    void show_error_msg_invalid_entry(const toml::node_view<const toml::node> entry, const char* prefix)
    // Show error message and quit this software.
    //
    // [Args]
    //   entry (const toml::node&): [IN] TOML entry.
    //
    {   // {{{

        // Prepare error message.
        std::stringstream ss;
        ss << prefix << entry.as_array();

        // Print error message and exit the function.
        print_error("Error", ss.str());

    }   // }}}

    void show_warn_msg_invalid_ctype(const StringView completion_type)
    // Show warning message and continue this software.
    //
    // [Args]
    //   completion_type (const StringView): [IN] Completion type string.
    //   
    {   // {{{

        // Prepare warning message.
        std::stringstream ss;
        ss << "Invalid completion type: " << completion_type << ". ";
        ss << "It must be one of 'carapace', 'command', 'grep', 'path', 'preview', or 'shell'. ";
        ss << "Falling back to 'path' completion type.";

        // Print warning message and continue the function.
        print_error("Warning", ss.str());

    }   // }}}

    Vector<String> parse_node_as_string_vector(const toml::array* array_node)
    // Convert a TOML array node to a vector of strings.
    //
    // [Args]
    //   array_node (const toml::array*): [IN] Pointer to the TOML array node.
    //
    // [Returns]
    //   (Vector<String>): Vector of strings.
    //
    {   // {{{

        Vector<String> result;

        // If the given array node is invalid, return the empty vector.
        if (not array_node->is_array()) return result;

        // Read pattern strings.
        std::transform(array_node->begin(), array_node->end(), std::back_inserter(result),
                       [](const toml::node& value) { return value.value_or(""); });

        return result;

    }   // }}}

    StringMap parse_node_as_string_map(const toml::node_view<const toml::node> node)
    // Convert a TOML array node to a map of string and string.
    //
    // [Args]
    //   node (const toml::node_view<const toml::node>): [IN] View of the TOML array node.
    //
    // [Returns]
    //   (StringMap): Map of string and string.
    //
    {   // {{{

        // Initialize the result map.
        StringMap result;

        // If the given array node is invalid, show error message and return the empty map.
        if (not node.is_array()) return result;

        for (SizeType idx = 0; idx < node.as_array()->size(); ++idx)
        {
            // Get the view of the entry node.
            const toml::node_view<const toml::node> entry = node[idx];

            // Check if the entry is an array and has exactly 2 items (key and values).
            if ((not entry.is_array()) or (entry.as_array()->size() != 2))
            { show_error_msg_invalid_entry(entry, "Invalid entry: "); continue; }

            // Check if the first item is a string (key) and the second item is a string (value).
            if ((not entry.as_array()->at(0).is_string()) or (not entry.as_array()->at(1).is_string()))
            { show_error_msg_invalid_entry(entry, "Invalid entry: "); continue; }

            // Register the key and value to the result map.
            result.emplace(entry.as_array()->at(0).value_or(""), entry.as_array()->at(1).value_or(""));
        }

        return result;

    }   // }}}

    Vector<Completion> parse_node_as_completion_vector(const toml::node_view<const toml::node> node)
    // Convert a TOML array node to a vector of Completion objects.
    //
    // [Args]
    //   node (const toml::node_view<const toml::node>): [IN] View of the TOML array node.
    //
    // [Returns]
    //   (Vector<Completion>): Vector of Completion objects.
    //
    {   // {{{

        // Initialize the result vector.
        Vector<Completion> result;

        // If the given array node is invalid, show error message and return the empty map.
        if (not node.is_array()) return result;

        for (SizeType idx = 0; idx < node.as_array()->size(); ++idx)
        {
            // Get the view of the entry node.
            const toml::node_view<const toml::node> entry = node[idx];

            // Show an error message and skip the entry if it is not an array or does not have exactly 3 items
            // (completion pattern, completion type, and optional string).
            if ((not entry.is_array()) or (entry.as_array()->size() != 3))
            { show_error_msg_invalid_entry(entry, "Invalid completion entry: "); continue; }

            // Show an error message and skip the entry if the first item is not an array (completion pattern),
            // the second item is not a string (completion type), or the third item is not a string (optional string).
            if ( (not entry.as_array()->at(0).is_array()  )
              or (not entry.as_array()->at(1).is_string() )
              or (not entry.as_array()->at(2).is_string() ))
            { show_error_msg_invalid_entry(entry, "Invalid completion entry: "); continue; }

            ////////////////////////////////////////////////////////////////////////////////////
            // Read completion pattern
            ////////////////////////////////////////////////////////////////////////////////////

            // Read pattern strings.
            Vector<String> pattern = parse_node_as_string_vector(entry.as_array()->at(0).as_array());

            ////////////////////////////////////////////////////////////////////////////////////
            // Read completion type
            ////////////////////////////////////////////////////////////////////////////////////

            // Get the node of the completion type.
            const char* ctype_strptr = entry.as_array()->at(1).value_or("");

            // Compute completion type.
            CompType ctype;
            if      (strcmp(ctype_strptr, "carapace") == 0) { ctype = CompType::CARAPACE; }
            else if (strcmp(ctype_strptr, "command")  == 0) { ctype = CompType::COMMAND;  }
            else if (strcmp(ctype_strptr, "grep")     == 0) { ctype = CompType::GREP;     }
            else if (strcmp(ctype_strptr, "path")     == 0) { ctype = CompType::PATH;     }
            else if (strcmp(ctype_strptr, "preview")  == 0) { ctype = CompType::PREVIEW;  }
            else if (strcmp(ctype_strptr, "shell")    == 0) { ctype = CompType::SHELL;    }
            else { show_warn_msg_invalid_ctype(ctype_strptr); ctype = CompType::PATH;     }

            ////////////////////////////////////////////////////////////////////////////////////
            // Read optional string
            ////////////////////////////////////////////////////////////////////////////////////

            // Get the node of the optional string.
            const char* opt_strptr = entry.as_array()->at(2).value_or("");

            // Register the triplet of completion pattern, completion type, and optional string.
            result.emplace_back(pattern, ctype, opt_strptr);
        }

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
        toml::node_view<const toml::node> node = table[section][value];

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

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Read the [PROMPT] section.
        ////////////////////////////////////////////////////////////////////////////////////////////

        else if ((section == "PROMPT") and (value == "ps1i" )) cfg.ps1i  = node.value_or(cfg.ps1i);
        else if ((section == "PROMPT") and (value == "ps1n" )) cfg.ps1n  = node.value_or(cfg.ps1n);
        else if ((section == "PROMPT") and (value == "ps2"  )) cfg.ps2   = node.value_or(cfg.ps2);
        else if ((section == "PROMPT") and (value == "ps_ex")) cfg.ps_ex = node.value_or(cfg.ps_ex);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Read the [KEYBIND] section.
        ////////////////////////////////////////////////////////////////////////////////////////////

        else if ((section == "KEYBIND") and (value == "candidate_completion_key")) cfg.cand_comp_key       = node.value_or(cfg.cand_comp_key);
        else if ((section == "KEYBIND") and (value == "history_completion_key"  )) cfg.hist_comp_key       = node.value_or(cfg.hist_comp_key);
        else if ((section == "KEYBIND") and (value == "plugin_trigger_keys"     )) cfg.plugin_trigger_keys = parse_node_as_string_map(node);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Read the [COMPLETION] section.
        ////////////////////////////////////////////////////////////////////////////////////////////

        else if ((section == "COMPLETION") and (value == "completions")) cfg.completions = parse_node_as_completion_vector(node);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Read the [PREVIEW] section.
        ////////////////////////////////////////////////////////////////////////////////////////////

        else if ((section == "PREVIEW") and (value == "previews")     ) cfg.previews      = parse_node_as_string_map(node);
        else if ((section == "PREVIEW") and (value == "preview_delim")) cfg.preview_delim = node.value_or(cfg.preview_delim);
        else if ((section == "PREVIEW") and (value == "preview_ratio")) cfg.preview_ratio = node.value_or(cfg.preview_ratio);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Config values for plugins
        ////////////////////////////////////////////////////////////////////////////////////////////

        // The section "PLUGIN_*" will be used for plugins, so ignore in RedAlien.
        else if (StringView(section).starts_with("PLUGIN_")) { /* Do nothing */ }

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
