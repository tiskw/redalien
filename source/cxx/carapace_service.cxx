////////////////////////////////////////////////////////////////////////////////////////////////////
/// C++ source file: carapace_service.cxx                                                        ///
////////////////////////////////////////////////////////////////////////////////////////////////////

// Include the primary header.
#include "carapace_service.hxx"

// Include the header of JSON library.
#include "json.hpp"
using json = nlohmann::json;

// Include the headers of custom modules.
#include "cmd_runner.hxx"
#include "utils.hxx"

////////////////////////////////////////////////////////////////////////////////////////////////////
// File-local functions
////////////////////////////////////////////////////////////////////////////////////////////////////

// Unnamed namespace for making classes and functions file-local.
namespace
{
    bool sort_func(const Tuple<StringView, StringView, StringView>& t1, const Tuple<StringView, StringView, StringView>& t2) noexcept
    // Sort the completion candidates based on their values and whether they are directories or files.
    //
    // [Args]
    //   t1 (const Tuple<StringView, StringView, StringView>&): [IN] First tuple to compare.
    //   t2 (const Tuple<StringView, StringView, StringView>&): [IN] Second tuple to compare.
    //
    // [Returns]
    //   (bool): True if the first tuple should come before the second tuple, false otherwise.
    //
    {   // {{{

        // Get the value of the pairs.
        const StringView s1 = std::get<0>(t1);
        const StringView s2 = std::get<0>(t2);

        // Determine if the values are directories (ending with '/').
        const bool is_dir1 = (s1.size() > 0) and (s1.back() == '/');
        const bool is_dir2 = (s2.size() > 0) and (s2.back() == '/');

        // Sort directories before files, and sort lexicographically within each group.
        if      (is_dir1 and is_dir2) return (s1 < s2);
        else if (is_dir1            ) return true;
        else if (            is_dir2) return false;
        else                          return (s1 < s2);

    }   // }}}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// CarapaceService: Constructors and destructors
////////////////////////////////////////////////////////////////////////////////////////////////////

CarapaceService::CarapaceService(const RedAlienConfig& cfg) : colors(cfg.colors)
{   // {{{

    // Determine the path to the "carapace" binary executable based on the current executable's path.
    this->path_carapace_bin = get_executable_path().parent_path() / "carapace";

    // If the "carapace" binary does not exist at the determined path, set the path to an empty string.
    if (not stdfs::exists(this->path_carapace_bin))
        this->path_carapace_bin = Path("");

}   // }}}

CarapaceService::~CarapaceService(void)
{ /* Do nothing. */ }

////////////////////////////////////////////////////////////////////////////////////////////////////
// CarapaceService: Member functions
////////////////////////////////////////////////////////////////////////////////////////////////////

Generator<Tuple<StringView, StringView, const char*>> CarapaceService::complete(const Vector<StringView>& tokens)
{   // {{{

    // Do nothing if the path to the "carapace" binary is not available.
    if (this->path_carapace_bin.empty()) co_return;

    // Do nothing if no tokens are provided.
    if (tokens.size() < 2) co_return;

    // Clean up the cache if the size of the cache exceeds the threshold.
    constexpr SizeType max_cache_entries = 256;
    if (this->cache.size() >= max_cache_entries)
        this->cache.clear();

    // Prepare the command to run "carapace" with the given tokens.
    uint64_t hash_val = hash(tokens[0]);
    for (SizeType idx = 1; idx < tokens.size(); ++idx)
        hash_val = hash(tokens[idx], hash("\xFF", hash_val));

    if (not this->cache.contains(hash_val))
    {
        // Prepare the command to run "carapace" with the given tokens:
        //     args = ["path/to/carapace", tokens[0], "export", tokens[0], tokens[1], ...]
        Vector<StringView> args = {this->path_carapace_bin.c_str(), tokens.front(), "export"};
        args.insert(args.end(), tokens.begin(), tokens.end());

        try
        {
            // Parse the command output as JSON.
            const json result_json = json::parse(run_command(args, RUN_COMMAND_GETOUT));

            // Accept only {"values": [...]}; anything else yields no candidates.
            if ((not result_json.is_object()) or (not result_json.contains("values")) or (not result_json.at("values").is_array()))
                co_return;

            // Initialize flags.
            bool is_dir_completion = false;

            // Parse the JSON output and extract completion candidates.
            Vector<Tuple<String, String, const char*>> candidates;
            for (const json& item : result_json["values"])
            {
                // Get the completion candidate and its display string from the JSON object.
                const String value   = item.value("value",   "");
                const String display = item.value("display", "");
                const String style   = item.value("style",   "");
                const String tag     = item.value("tag",     "");

                // Check if the completion candidate is a directory based on its tag.
                if ((not is_dir_completion) and (tag == "files"))
                    is_dir_completion = true;

                // Register the completion candidate and its display string in the cache.
                candidates.emplace_back(value, display, this->get_color(style));
            }

            // Store the completion candidates in the cache for future use.
            this->cache[hash_val] = std::move(candidates);

            // Sort list results.
            if (is_dir_completion)
                std::sort(this->cache[hash_val].begin(), this->cache[hash_val].end(), sort_func);
        }
        // If an error occurs while parsing the JSON output, return without yielding any completion candidates.
        catch (const json::exception&)
        {
            co_return;
        }
    }

    // Yield the completion candidates that match the last token.
    for (const auto& [value, display, style] : this->cache[hash_val])
        if (value.starts_with(tokens.back()))
            co_yield {value, display, style};

}   // }}}

const char* CarapaceService::get_color(StringView style) const
{   // {{{

    // The style string returned from "carapace" is a space-separated string of style names.
    // See <https://carapace-sh.github.io/carapace-bin/style.html> for more details.
    // The following code supports only the color name styles.
    for (const StringView sv : split(style, " "))
        if (auto it = colors.find(sv); it != colors.end())
            return it->second.c_str();
    return "";

}   // }}}

// vim: expandtab tabstop=4 shiftwidth=4 fdm=marker
