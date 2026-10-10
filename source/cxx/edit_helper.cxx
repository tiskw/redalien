////////////////////////////////////////////////////////////////////////////////////////////////////
/// C++ source file: edit_helper.cxx                                                             ///
////////////////////////////////////////////////////////////////////////////////////////////////////

// Include the primary header.
#include "edit_helper.hxx"

// Include STL headers.
#include <algorithm>
#include <regex>

// Include the headers of custom modules.
#include "cmd_runner.hxx"
#include "error.hxx"
#include "path_x.hxx"
#include "preview.hxx"
#include "string_utils.hxx"
#include "tokenizers.hxx"
#include "utils.hxx"

////////////////////////////////////////////////////////////////////////////////////////////////////
// File-local functions
////////////////////////////////////////////////////////////////////////////////////////////////////

// Unnamed namespace for making classes and functions file-local.
namespace
{
    using RegEx = std::regex;

    std::shared_future<Vector<String>> shared_future_cache_commands;
    // Shared future instance of the command cache.

    std::shared_future<Vector<Vector<RegEx>>> shared_future_vec_patterns_regex;
    // Cache of compiled regular expression patterns for completion matching.

    std::once_flag flag_init_shared_futures;
    // A flag for one-time initialization of shared future instances.

    String colorize_name(StringView name, const String& query_key, const char* color_code)
    // Colorize the file name based on the file type and the user input query key.
    //
    // [Args]
    //   name       (const String&): [IN] File name to be colorized.
    //   query_key  (const String&): [IN] User input query key for colorization.
    //   color_code (const char*)  : [IN] Color code for the file type.
    //
    // [Returns]
    //   (String): Colorized file name for display.
    //
    {   // {{{

        // Returns without query colorization if the query key is empty or the query key is invalid.
        if (query_key.empty() or name.size() < query_key.size())
            return std::format("{}{}\x1B[0m", color_code, name);

        // Colorize the matched query key.
        const StringView sv1 = name.substr(0, query_key.size());
        const StringView sv2 = name.substr(query_key.size());
        return std::format("\x1B[35m{}\x1B[0m{}{}\x1B[0m", sv1, color_code, sv2);

    }   // }}}

    Vector<String> columnize(const Vector<StringView>& texts, Size area_size, uint16_t padding)
    // Columnize the given string vector.
    //
    // [Args]
    //   texts     (const Vector<StringView>&): [IN] Input string vector.
    //   area_size (Size)                     : [IN] Maximum size of display (columns and rows).
    //   padding   (uint16_t)                 : [IN] Padding width between columns.
    //
    // [Returns]
    //   (String): Columnized string.
    //
    {   // {{{

        constexpr auto get_shape = [](const Vector<uint16_t>& ws, uint16_t width, uint16_t margin, uint16_t n_rows) noexcept -> Tuple<uint16_t, bool>
        // Compute shape of column style display.
        //
        // [Args]
        //   ws     (const Vector<uint16_t>&): [IN] Length of each text.
        //   width  (uint16_t)               : [IN] Maximum width of display.
        //   margin (uint16_t)               : [IN] Minimum margin between each text.
        //   n_rows (uint16_t)               : [IN] Number of rows of the display.
        //
        // [Returns]
        //   (Tuple<uint16_t, bool>): A pair of (number of columns, true if all texts can be shown).
        {
            uint16_t wid_total = 0;

            // Create columns and append them to each line.
            for (uint16_t col = 0; true; ++col)
            {
                // Compute start/end index of the texts used in the current column
                const size_t idx_bgn = col * n_rows;
                const size_t idx_end = min(idx_bgn + n_rows, ws.size());

                // Compute maximum width of texts used in the current column.
                const uint16_t wid_max = (idx_end > idx_bgn) ? *std::max_element(ws.begin() + idx_bgn, ws.begin() + idx_end) : 0;

                // Increment of width by this column.
                const uint16_t wid_inc = ((col > 0) ? margin : 0) + wid_max;

                // Exit if the current width exceeds the maximum width.
                if ((wid_total + wid_inc) >= width)
                    return {max(static_cast<uint16_t>(1), col), false};

                // Exit if all text was used.
                if (idx_end == ws.size())
                    return {col + 1, true};

                // Update current width.
                wid_total += wid_inc;
            }
        };

        constexpr auto get_optimal_shape = [get_shape](const Vector<uint16_t>& ws, Size area_size, uint16_t margin) noexcept -> Size
        // Compute optimal shape (rows and columns) of column style display.
        //
        // [Args]
        //   ws  (const Vector<uint16_t>): [IN] Input texts.
        //   width  (uint16_t)           : [IN] Maximum width of display.
        //   height (uint16_t)           : [IN] Maximum height of display.
        //   margin (uint16_t)           : [IN] Minimum margin between each text.
        //
        // [Returns]
        //   (Size): A pair of rows and columns of the optimal shape.
        {
            for (uint16_t row = 1; row < area_size.rows; ++row)
            {
                // Compute shape for each row.
                const auto [col, finished] = get_shape(ws, area_size.cols, margin, row);

                // Immediately determine optimal shape if all texts can be shown.
                if (finished)
                    return {col, row};
            }

            // Compute column if row is equal with the maximum height.
            const auto [col, _] = get_shape(ws, area_size.cols, margin, area_size.rows);

            return {col, area_size.rows};
        };

        // Clip the padding to ensure it does not exceed the maximum width of the display.
        padding = clip(padding, static_cast<uint16_t>(0), static_cast<uint16_t>(area_size.cols - 1));

        // Prepare output lines.
        Vector<String> lines;
        for (uint16_t row = 0; row < area_size.rows; ++row)
            lines.emplace_back("");

        // Do nothing if no text is given.
        if (texts.size() == 0)
            return lines;

        // Get width of each text.
        Vector<uint16_t> ws = transform<StringView, uint16_t>(texts, width);

        // Compute optimal shape (rows and columns).
        const Size column_shape = get_optimal_shape(ws, area_size, padding);

        // Initialize total width.
        uint16_t width_total = 0;

        // Create columns and append them to each line.
        for (uint16_t col = 0; col < column_shape.cols; ++col)
        {
            // Compute start/end index of the texts used in the current column
            const size_t idx_bgn = col * column_shape.rows;
            const size_t idx_end = min(idx_bgn + column_shape.rows, ws.size());

            // Compute maximum width of texts used in the current column.
            const uint16_t wid_max = (idx_end > idx_bgn) ? *std::max_element(ws.cbegin() + idx_bgn, ws.cbegin() + idx_end) : 0;

            // Append texts to each line.
            for (uint16_t idx = idx_bgn; idx < idx_end; ++idx)
            {
                lines[idx % column_shape.rows] += texts[idx];

                // Append margin whitespaces.
                if (col < (column_shape.cols - 1))
                    lines[idx % column_shape.rows] += String(wid_max + padding - ws[idx], ' ');
            }

            // Update total width.
            width_total += wid_max + padding;

            // Exit function if the current width exceeds the given width.
            if (width_total > area_size.cols) break;
        }

        return lines;

    }   // }}}

    Vector<Vector<RegEx>> compile_regex_patterns(const Vector<Completion>& completions)
    // Compile the regular expression patterns in the config file and store them in the given vector.
    //
    // [Args]
    //   completions (const auto&): [IN] Vector of completion targets in the config file.
    //
    // [Returns]
    //   (Vector<Vector<std::regex>>): Vector of compiled regular expression patterns.
    //
    {   // {{{

        // Initialize the output vector.
        Vector<Vector<RegEx>> vec_regex;

        // Compile the regular expression patterns in the config file.
        for (const auto& [patterns, comp_type, option] : completions)
        {
            Vector<std::regex> patterns_regex;
            for (const String& pattern : patterns)
            {
                try
                {
                    patterns_regex.emplace_back(RegEx(pattern));
                }
                catch (const std::regex_error& e)
                {
                    // Print error message and exit the function.
                    print_error("Warning", std::format("Invalid regex pattern in completion pattern: '{}' ({})", pattern, e.what()));

                    // If the regex pattern is invalid, use a regex that matches nothing to avoid crashing the program.
                    patterns_regex.emplace_back(RegEx(R"(^\b$)"));
                }
            }

            vec_regex.push_back(std::move(patterns_regex));
        }

        return vec_regex;

    }   // }}}

    Vector<String> get_available_commands(const Path& path_cmnd_info, const Path& path_bash_info)
    // Get a list of all available commands in Bash.
    //
    // [Args]
    //   path_cmnd_info (const Path&): [IN] Path to RedAlien's command info file.
    //   path_bash_info (const Path&): [IN] Path to RedAlien's bash info file.
    //
    // [Returns]
    //   (Vector<String>): List of available command names.
    //
    {   // {{{

        // Initialize the output vector.
        Vector<String> result;

        // Append the Bash bash info.
        for (const String& line : readline(path_bash_info.c_str()))
            if ((not line.empty()) and isalpha(line[0]))
                result.emplace_back(strip(line));

        // Append the path commands info.
        for (const String& line : readline(path_cmnd_info.c_str()))
            if ((not line.empty()) and isalpha(line[0]))
                result.emplace_back(strip(line));

        // Sort and remove duplicated command names.
        deduplicate(result);

        return result;

    }   // }}}

    bool match(const Vector<String>& patterns, const Vector<StringView>& tokens, const Vector<std::regex>& patterns_regex)
    // Return True if the given tokens matched with the given patterns.
    // The arguments `tokens` is a list of strings, and the argument `patterns`
    // are list of regular expression strings with the following extra special commands:
    //
    //   * "FILE": existing file path
    //   * ">>"  : skip tokens (available only 1 time in one patterns)
    //
    // [Args]
    //   patterns (const Vector<String>&): [IN] List of regular expression strings.
    //   tokens   (const Vector<String>&): [IN] List of strings to be matched.
    //
    // [Returns]
    //   (bool): True is the given tokens matched with the given patterns.
    //
    {   // {{{

        // Initialize token index.
        SizeType index_token = 0;

        // Run the for-loop based on the pattern index.
        for (SizeType index_pattern = 0; index_pattern < patterns.size(); ++index_pattern)
        {
            // If the number of patterns is longer than the number of tokens
            // (i.e. token finished but pattern is exists yet), then returns false.
            if (index_token >= tokens.size())
                return false;

            // Select target pattern and token.
            const String&    pattern = patterns[index_pattern];
            const StringView token   = tokens[index_token];

            // Select the compiled regular expression pattern.
            const std::regex& pattern_regex = patterns_regex[index_pattern];

            // Case 1: pattern is ">>".
            if (pattern == ">>")
                index_token = tokens.size() - patterns.size() + index_pattern;

            // Case 2: pattern is "FILE".
            else if (pattern == "FILE")
            {
                // If the file does not exist, then returns false.
                std::error_code ec;
                if (not stdfs::exists(token, ec) or ec)
                    return false;

                // Otherwise, the file exists, do nothing and continue to the next token.
            }

            // Case 3: others.
            else if (not regex_match(token.begin(), token.end(), pattern_regex))
                return false;

            ++index_token;
        };

        // No unprocessed tokens remains if the token matches with the pattern.
        return {index_token == tokens.size()};

    }   // }}}

    Tuple<CompType, String> get_target(const Vector<StringView>& tokens, const Vector<Completion>& completions, const Vector<Vector<std::regex>>& vec_patterns_regex)
    // Returns a pair of target completion type and its optional string.
    //
    // [Args]
    //   tokens (const Vector<String>&): [IN] List of strings to be matched.
    //
    // [Returns]
    //   (CompType): Target completion type.
    //   (String)  : Optional string of the target completion.
    //
    {   // {{{

        for (SizeType idx = 0; idx < completions.size(); ++idx)
        {
            // Get the completion target.
            const auto& [patterns, comp_type, option] = completions[idx];

            // Get the compiled regular expression patterns.
            const Vector<std::regex>& patterns_regex = vec_patterns_regex[idx];

            // Check if the given tokens match with the current pattern.
            const bool is_matched = match(patterns, tokens, patterns_regex);

            // Returns the current completion type and its optional string.
            if (is_matched)
                return {comp_type, option};
        }

        return {CompType::NONE, String("")};

    }   // }}}

    const char* get_color(const DirEntry& entry) noexcept
    // Get color code based on the file type. No file system access is performed here,
    // because the file type is already resolved by PathX::listdir.
    //
    // [Args]
    //   entry (const DirEntry&): [IN] Directory entry.
    //
    // [Returns]
    //   (const char*): Color code for the file type.
    //
    {   // {{{

        if (entry.is_dir)  return "\x1B[94m";  // Directory.
        if (entry.is_exec) return "\x1B[92m";  // Executable file.
        return "\x1B[0m";                      // Others.

    }   // }}}

    void init_shared_futures(const Path& outdir, const RedAlienConfig& cfg)
    // Initialize shared future instances for caching command names and compiled regular expression patterns.
    //
    // [Args]
    //   outdir (const Path&)          : [IN] Path to output directory.
    //   cfg    (const RedAlienConfig&): [IN] Config data for initializing the shared futures.
    //
    {   // {{{

        // Get the paths of the command info and bash info files.
        const Path path_cmnd_info = outdir / "cmnd_info.txt";
        const Path path_bash_info = outdir / "bash_info.txt";

        // Create the cache of available command names.
        shared_future_cache_commands = launch_async(get_available_commands, path_cmnd_info, path_bash_info).share();

        // Create the cache of compiled regular expression patterns for completion matching.
        shared_future_vec_patterns_regex = launch_async(compile_regex_patterns, cfg.completions).share();

    }   // }}}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// EditHelper: Constructors and destructors
////////////////////////////////////////////////////////////////////////////////////////////////////

EditHelper::EditHelper(uint16_t rows, uint16_t cols, const Path& outdir, const RedAlienConfig& cfg)
    : area_size(cols, rows), carapace_service(cfg), column_padding(cfg.column_padding),
      completions(cfg.completions), previews(cfg.previews), preview_delim(cfg.preview_delim),
      preview_ratio(cfg.preview_ratio)
{   // {{{

    // Initialize shared future instances for caching command names and compiled regular expression patterns.
    std::call_once(flag_init_shared_futures, &init_shared_futures, outdir, cfg);

}   // }}}

////////////////////////////////////////////////////////////////////////////////////////////////////
// EditHelper: Member functions
////////////////////////////////////////////////////////////////////////////////////////////////////

const Vector<String>& EditHelper::candidate(StringView lhs)
{   // {{{

    // Check if the completion candidates for the given lhs are already cached.
    if (const auto it = this->cache_cands_lhs.find(lhs); it != this->cache_cands_lhs.end())
    {
        this->cands = &it->second.cands;
        this->lines = &it->second.lines;
        return *this->lines;
    }

    // Clear the caches if the number of cache entries exceeds the maximum limit.
    // This process must be done before the entry is allocated, because clear() invalidates all pointers in the map.
    constexpr SizeType max_cache_entries = 256;
    if (this->cache_cands_lhs.size() >= max_cache_entries)
        this->cache_cands_lhs.clear();

    // Split the given text (left hand side of the cursor) to tokens.
    // Drop white-space tokens and convert String to String.
    Vector<StringView> tokens;
    for (const StringView token : tokenize(lhs, TOKENIZE_PLAIN))
        tokens.emplace_back(token);

    // Add empty token if the editing line ends with white-space (except inside an open quote).
    if ((lhs.size() > 0) and (lhs.back() == ' ') and (tokens.empty() or not is_open_quote(tokens.back())))
        tokens.push_back(StringView(""));

    // Strip the quotes of the last token, because candidates are computed from the raw string.
    if (not tokens.empty())
        tokens.back() = unquote(tokens.back());

    // Get completion type and its optional string.
    const auto& [comp_type, option] = get_target(tokens, this->completions, shared_future_vec_patterns_regex.get());

    // Select the instance of cands and lines.
    if (comp_type == CompType::NONE)
    {
        // If the completion type is NONE, use the entry for none.
        this->cands = &this->none_cache_entry.cands;
        this->lines = &this->none_cache_entry.lines;
    }
    else
    {
        // Otherwise, allocate a new entry in the cache of lhs.
        CandCacheEntry& entry = this->cache_cands_lhs[String(lhs)];
        this->cands = &entry.cands;
        this->lines = &entry.lines;

        // NOTE: This implementation may create an empty cache entry if an error occurs during the
        // completion process. Once such an entry is created, the completion process is not retried.
        // However, the author believes the current approach is appropriate for the following reasons:
        //   - We could avoid creating an empty cache entry by first storing the completion results in
        //     a local variable and then adding them to the cache. However, we have observed that this
        //     change introduces additional latency into the completion process and negatively affects
        //     the responsiveness of completion display on the initial keystroke.
        //   - The impact of an empty cache entry is limited to the lifetime of the "redalien" process
        //     and is short-lived. It does not persist for the duration of the entire Bash session.
    }

    // Compute completion candidates that will be displayed to users.
    this->cands->clear();
    switch (comp_type)
    {
        case CompType::CARAPACE : this->cands_carapace (tokens);         break;
        case CompType::COMMAND  : this->cands_command  (tokens, option); break;
        case CompType::GREP     : this->cands_grep     (tokens, option); break;
        case CompType::PATH     : this->cands_filepath (tokens);         break;
        case CompType::PREVIEW  : this->cands_filepath (tokens);         break;
        case CompType::SHELL    : this->cands_shell    (tokens, option); break;
        case CompType::NONE     : this->cands_filepath (tokens);         break;
    }

    // Convert completion candidates to lines for display.
    this->lines_from_cands(*this->cands);

    // Update completion lines using preview info.
    if (comp_type == CompType::PREVIEW)
        this->cands_preview(tokens);

    return *this->lines;

}   // }}}

String EditHelper::complete(StringView lhs) const
{   // {{{

    constexpr auto get_common_substr = [](const Vector<String>& texts) noexcept -> StringView
    // Get common substring of the given strings.
    //
    // [Args]
    //   texts (const Vector<String>&): [IN] List of strings.
    //
    // [Returns]
    //   (String): Common string in the given list of strings.
    {
        // Returns empty string if the size of the given text list is zero.
        if (texts.size() == 0) return StringView("");

        // Returns entire string if the size of the given text list is one.
        if (texts.size() == 1) return StringView(texts[0]);

        // Find minimum length of the given texts.
        SizeType min_size = texts[0].size();
        for (SizeType n = 1; n < texts.size(); ++n)
            min_size = min(min_size, texts[n].size());

        // Check consistency for each character and append to the result string.
        for (SizeType m = 0; m < min_size; ++m)
        {
            // Check the character consistency.
            for (SizeType n = 1; n < texts.size(); ++n)
                if (texts[n][m] != texts[0][m])
                    return StringView(texts[0].c_str(), m);
        }

        return StringView(texts[0].c_str(), min_size);
    };

    // Split the given text (left hand side of the cursor) to tokens.
    Vector<StringView> tokens;
    for (const StringView token : tokenize(lhs, TOKENIZE_KEEP_WS))
        tokens.emplace_back(token);

    // Do nothing if token is empty.
    if (tokens.size() == 0) return String(lhs);

    // Do nothing if candidate() has not been called yet.
    if (this->cands == nullptr) return String(lhs);

    // Get number of completion candidates.
    const size_t num_cands = this->cands->size();

    // Do nothing if no candidate given.
    if (num_cands == 0) return String(lhs);

    // Get the first candidate (this item will be used many times in the following).
    const Pair<String, String>& cand_first = (*this->cands)[0];

    // The raw token (possibly quoted) is replaced, while the unquoted one is used for matching.
    const StringView raw_token = tokens.back();
    const StringView last_token = unquote(raw_token);
    StringView lhs_without_last_token = (
        raw_token.starts_with(" ") or raw_token.starts_with("\t") or not cand_first.first.starts_with(last_token)
    ) ? StringView(lhs) : StringView(lhs.data(), lhs.size() - raw_token.size());

    // Compute completion string.
    if ((num_cands == 1) and cand_first.first.ends_with('/'))
    {
        // Extra slash will be added when directory path is completed.
        // However, slash is already added to the completion token,
        // therefore just adding the completion token is enough.
        return String(lhs_without_last_token) + shell_quote(cand_first.first, false);
    }
    else if (num_cands == 1)
    {
        // Add extra white-space at the end if number of completion candidate is one.
        return String(lhs_without_last_token) + shell_quote(cand_first.first, true) + ' ';
    }
    else
    {
        // Create an array of completion strings and find the common substring among them.
        constexpr auto get_first = [](const Pair<String, String>& pair) noexcept -> String { return pair.first; };
        Vector<String> keys = transform<Pair<String, String>, String>(*this->cands, get_first);
        return String(lhs_without_last_token) + shell_quote(get_common_substr(keys), false);
    }

}   // }}}

////////////////////////////////////////////////////////////////////////////////////////////////////
// EditHelper: Private functions
////////////////////////////////////////////////////////////////////////////////////////////////////

void EditHelper::cands_command(const Vector<StringView>& tokens, const String& option)
{   // {{{

    constexpr auto description = [](StringView cmd, StringView token) -> String
    // Returns colorized command name for description of the compretion.
    //
    // [Args]
    //   cmd   (const String&): [IN] Command name to be displayed.
    //   token (const String&): [IN] User input token for the command name.
    //
    // [Returns]
    //   (String): Colorized command name for display.
    {
        if (token.empty() or cmd.size() < token.size())
            return String(cmd);

        return std::format("\x1B[35m{}\x1B[0m{}", token, cmd.substr(token.size()));
    };

    // The `option` should be empty string.
    if (option.size() > 0) return;

    // Get query token.
    const StringView token = tokens[0];

    // Prepare command cache.
    if (this->cache_commands.size() == 0)
        for (const String& cmd : shared_future_cache_commands.get())
            this->cache_commands.emplace_back(cmd);

    // Filter matched command names.
    for (const String& cmd : this->cache_commands)
        if (cmd.starts_with(token))
            this->cands->emplace_back(String(cmd), description(cmd, token));

}   // }}}

void EditHelper::cands_carapace(const Vector<StringView>& tokens)
{   // {{{

    // Split user input token to a tuple of:
    //   * directory path to be searched,
    //   * query string for filtering search result.
    const auto [query_dir, query_key] = split_to_target_and_query(tokens);

    // Colorize the completion candidates based on the style information from carapace,
    // and append them to the candidate list.
    for (const auto& [path, name, color_code] : this->carapace_service.complete(tokens))
        this->cands->emplace_back(path, colorize_name(name, query_key, color_code));

    // If no candidates found from carapace, then fallback to file path completion.
    if (this->cands->empty()) this->cands_filepath(tokens);

}   // }}}

void EditHelper::cands_filepath(const Vector<StringView>& tokens)
{   // {{{

    // Split user input token to a tuple of:
    //   * directory path to be searched,
    //   * query string for filtering search result.
    const auto [query_dir, query_key] = split_to_target_and_query(tokens);

    // List the matched entries. The prefix filtering, the hidden file filtering,
    // and the time limit are all handled inside PathX::listdir.
    const ListdirResult listing = query_dir.listdir(query_key);

    for (const DirEntry& entry : listing.entries)
    {
        // Compute path of the target file.
        const Path path = query_dir / entry.name;

        // Append query string and colorized display string.
        this->cands->emplace_back(path.string(), colorize_name(entry.name, query_key, get_color(entry)));
    }

}   // }}}

void EditHelper::cands_grep(const Vector<StringView>& tokens, const String& option)
{   // {{{

    // Get query token.
    const StringView token = (tokens.size() > 0) ? tokens.back() : StringView("");

    // Find the position of the first tab character in the option string.
    // If the tab character is not found, then the option string is invalid.
    const SizeType sep = option.find('\t');
    if (sep == String::npos) return;

    // Get the target file path.
    const String path = expand_tilde(option.substr(0, sep));

    // Get the regular expression pattern.
    Optional<RegEx> pattern;
    try
    {
        pattern.emplace(option.substr(sep + 1));
    }
    catch (const std::regex_error& e)
    {
        print_error("Warning", std::format("Invalid regex pattern in grep option: '{}' ({})", option.substr(sep + 1), e.what()));
        return;
    }

    // Create a matching result instance.
    std::smatch match;

    // Read lines from the file and print the match result if the pattern is matched.
    for (const String& line : readline(path.c_str()))
    {
        // Skip if not matched with the pattern.
        if (not std::regex_search(line, match, *pattern))
            continue;

        // Get the target string that will be displayed as a completion candidate.
        String target = (match.size() > 1) ? match[1] : line;

        if (target.starts_with(token))
            this->cands->emplace_back(target, target);
    }

}   // }}}

void EditHelper::cands_preview(const Vector<StringView>& tokens)
{   // {{{

    constexpr auto get_last_nonwhitespace_token = [](const Vector<StringView>& tokens) noexcept -> StringView
    // Returns last non-whitespace token.
    //
    // [Args]
    //   tokens (Vector<StringView>&): [IN] Target tokens.
    //
    // [Returns]
    //   (String): Non-whitespace token.
    {
        // Search tokens from the tail.
        for (PtrDiff idx = tokens.size() - 1; idx >= 0; --idx)
            if (tokens[idx].size() > 0 and tokens[idx][0] != ' ')
                return tokens[idx];

        // Returns brank string if not found.
        return StringView("");
    };

    // Get the target file path that is a last non-white-space token.
    const StringView path = get_last_nonwhitespace_token(tokens);

    // Compute width of the preview window.
    const uint16_t width_prev = static_cast<uint16_t>(max(static_cast<int32_t>(this->area_size.cols)
                                                        - static_cast<int32_t>(this->area_size.cols * this->preview_ratio)
                                                        - static_cast<int32_t>(this->preview_delim.size()), 1));

    // Get preview result.
    Vector<String> preview_lines = preview(path, this->area_size.rows, this->previews);

    // Append preview lines to the current completion lines.
    for (size_t idx = 0; (idx < preview_lines.size()) and (idx < (size_t) this->area_size.rows); ++idx)
    {
        // Get the target line.
        String& line = (*this->lines)[idx];

        // Get the width of the current line.
        const uint16_t width_line = width(line);

        // Clip the line to the half-width `w`.
        if (width_line > width_prev)
            line = String(textclip(line, width_prev));

        // Add extra white-space if the line is shorter than the half-width `w`.
        else if (width_line < width_prev)
            line += String(width_prev - width_line, ' ');

        // Append preview delimiter and line to the target line.
        line += String("\033[m") + preview_delim + preview_lines[idx];

        // Clip the target line again.
        line = String(textclip(line, this->area_size.cols - 1));
    }

}   // }}}

void EditHelper::cands_shell(const Vector<StringView>& tokens, const String& option)
{   // {{{

    // Get the target token.
    const StringView token = (tokens.size() > 0) ? tokens.back() : StringView("");

    // Tokenize the given command option and replace placeholder if exists.
    Vector<String> cmd_tokens;
    for (const StringView cmd_token : tokenize_with_placeholder_replacement(option, {}, TOKENIZE_DEQUOTE))
        cmd_tokens.emplace_back(cmd_token);

    // Run specified command.
    const String output = run_command(cmd_tokens, RUN_COMMAND_GETOUT);

    for (StringView line : split(output, "\n"))
    {
        // Strip line.
        line = strip(line);

        // Get the position of the first whitespace in the line.
        SizeType pos_ws = line.find(' ');

        // Get 1st token of each line.
        const StringView line_1st_token = (pos_ws != String::npos) ? StringView(line.data(), pos_ws) : StringView(line);

        // Register matched output lines.
        if (line_1st_token.starts_with(token))
            this->cands->emplace_back(line_1st_token, line);
    }

}   // }}}

void EditHelper::lines_from_cands(const Vector<Pair<String, String>>& cands)
{   // {{{

    constexpr auto get_second_view = [](const Pair<String, String>& pair) noexcept -> StringView
    // Returns the second element of the given pair as StringView.
    //
    // [Args]
    //   pair (const Pair<String, String>&): [IN] Target pair.
    //
    // [Returns]
    //   (StringView): The second element of the given pair as StringView.
    {
        return StringView(pair.second);
    };

    // Get descriptions of the completion candidates.
    Vector<StringView> texts = transform<Pair<String, String>, StringView>(cands, get_second_view);

    // Format descriptions in a column style.
    this->lines->clear();
    for (const String& line : columnize(texts, this->area_size, this->column_padding))
        this->lines->emplace_back(line);

}   // }}}

////////////////////////////////////////////////////////////////////////////////////////////////////
// EditHelper: Private static member functions
////////////////////////////////////////////////////////////////////////////////////////////////////

// vim: expandtab tabstop=4 shiftwidth=4 fdm=marker
