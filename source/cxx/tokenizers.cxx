////////////////////////////////////////////////////////////////////////////////////////////////////
/// C++ source file: tokenizers.cxx                                                              ///
////////////////////////////////////////////////////////////////////////////////////////////////////

// Include the primary header.
#include "tokenizers.hxx"

// Include STL headers.
#include <algorithm>
#include <cctype>

// Include the headers of custom modules.
#include "utils.hxx"

////////////////////////////////////////////////////////////////////////////////////////////////////
// File-local functions
////////////////////////////////////////////////////////////////////////////////////////////////////

// Unnamed namespace for making classes and functions file-local.
namespace
{
    String replace_path_plugin(StringView token, const Vector<Path>& path_plugin_candidates)
    // Replace "{path_plugin}" in the given token with an appropriate path.
    //
    // [Args]
    //   token                  (StringView)         : [IN] Target token to be replaced.
    //   path_plugin_candidates (const Vector<Path>&): [IN] Candidate paths for "{path_plugin}".
    //
    // [Returns]
    //   (String): Replaced token.
    //
    {   // {{{

        for (const Path& path_plugin : path_plugin_candidates)
        {
            // Replace "{path_plugin}" in the token with the candidate path.
            String token_new = replace(token, "{path_plugin}", path_plugin.string());

            // Check if the replaced token is a valid file path.
            std::error_code ec1, ec2;
            if (stdfs::exists(token_new, ec1) and (not ec1) and stdfs::is_regular_file(token_new, ec2) and (not ec2))
                return token_new;
        }

        // If no candidate path is valid, return the original token.
        return String(token);

    }   // }}}

    StringViewConstIter get_string_token(StringViewConstIter iter, StringViewConstIter iter_end, const char quote)
    // Get string token that is quoted by single/double quote.
    // This function is used in the "tokenize" function.
    //
    // [Args]
    //   iter     (StringViewConstIter): [IN]  Start iterator.
    //   iter_end (StringViewConstIter): [IN]  End iterator.
    //   quote    (const char)         : [IN]  Quote character.
    //
    // [Returns]
    //   (StringViewConstIter): Iterator pointing to the end of the token.
    //
    {   // {{{

        ++iter;
        while ((iter != iter_end) and (*iter != quote))
            ++iter;
        return (iter != iter_end) ? ++iter : iter;

    }   // }}}

    StringViewConstIter get_wspace_token(StringViewConstIter iter, StringViewConstIter iter_end)
    // Get white-space token.
    // This function is used in the "tokenize" function.
    //
    // [Args]
    //   iter     (StringViewConstIter): [IN]  Start iterator.
    //   iter_end (StringViewConstIter): [IN]  End iterator.
    //
    // [Returns]
    //   (StringViewConstIter): Iterator pointing to the end of the token.
    //
    {   // {{{

        while ((iter != iter_end) and ((*iter == ' ') or (*iter == '\t')))
            ++iter;
        return iter;

    }   // }}}

    StringViewConstIter get_normal_token(StringViewConstIter iter, StringViewConstIter iter_end)
    // Get normal token.
    // This function is used in the "tokenize" function.
    //
    // [Args]
    //   iter     (StringViewConstIter): [IN]  Start iterator.
    //   iter_end (StringViewConstIter): [IN]  End iterator.
    //
    // [Returns]
    //   (StringViewConstIter): Iterator pointing to the end of the token.
    //
    {   // {{{

        while ((iter != iter_end) and (*iter != ' ') and (*iter != '\t'))
            ++iter;
        return iter;

    }   // }}}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Public functions
////////////////////////////////////////////////////////////////////////////////////////////////////

Generator<StringView> tokenize(StringView sv, TokenizeOption option)
{   // {{{

    // Parse the tokenization options.
    const bool keep_whitespaces = (option & TOKENIZE_KEEP_WS) != 0;
    const bool dequote_tokens   = (option & TOKENIZE_DEQUOTE) != 0;

    // Initialize iterators.
    StringViewConstIter iter = sv.cbegin();

    while (iter != sv.cend())
    {
        //
        StringViewConstIter iter_bgn = iter;

        // Read a token.
        switch (*iter_bgn)
        {
            case '\'': iter = get_string_token(iter, sv.cend(), '\''); break;  // String token.
            case '"' : iter = get_string_token(iter, sv.cend(), '"');  break;  // String token.
            case ' ' : iter = get_wspace_token(iter, sv.cend());       break;  // Whitespace token.
            case '\t': iter = get_wspace_token(iter, sv.cend());       break;  // Whitespace token.
            default  : iter = get_normal_token(iter, sv.cend());       break;  // Others.
        }

        // Skip white-space tokens if the option to keep white-spaces is not set.
        if ((not keep_whitespaces) and ((*iter_bgn == ' ') or (*iter_bgn == '\t')))
            continue;

        // Get the token string view.
        StringView token = StringView(iter_bgn, iter - iter_bgn);

        // De-quote the token if the option to de-quote tokens is set.
        // Unclosed quotes (e.g. "'abc") are kept as-is.
        if (dequote_tokens and (not is_open_quote(token)))
            token = unquote(token);

        co_yield token;
    }

}   // }}}

Generator<String> tokenize_with_placeholder_replacement(StringView command, const StringMap& extra, TokenizeOption option)
{   // {{{

    // Get home directory path from the environment variable.
    const char* home = std::getenv("HOME");

    // Define candidate paths for the "{path_plugin}".
    const Vector<Path> path_plugin_candidates = {
        ((home != nullptr) ? Path(home) : Path(".")) / ".config/redalien/plugins",
        ((home != nullptr) ? Path(home) : Path(".")) / ".local/share/redalien/plugins",
    };

    // Tokenize the command string.
    for (const StringView token_view : tokenize(command, option))
    {
        String token(token_view);

        // Replace "{path_plugin}" with an appropriate path.
        if (token.starts_with("{path_plugin}"))
            token = replace_path_plugin(token, path_plugin_candidates);

        // Replace the token with the value in the "extra" map.
        else if (extra.contains(token))
            token = extra.at(token);

        // Replace tilde to home directory path.
        if (token.starts_with("~") and (home != nullptr))
            token = String(home) + String(token.substr(1));

        co_yield token;
    }

}   // }}}

bool is_open_quote(StringView token) noexcept
// Returns true if the token starts with a quote that is not closed yet (e.g. "'my fi").
//
// [Args]
//   token (StringView): [IN] Token to be checked.
//
// [Returns]
//   (bool): True if the token starts with a quote that is not closed yet.
//
{   // {{{

    if      (token.empty()   ) return false;
    else if (token[0] == '\'') return (token.size() == 1) or (token.back() != '\'');
    else if (token[0] == '"' ) return (token.size() == 1) or (token.back() != '"');
    else                       return false;

}   // }}}

StringView unquote(StringView token) noexcept
// Strip the surrounding quotes of a token ("'my fi" -> "my fi", "'a b'" -> "a b").
//
// [Args]
//   token (StringView): [IN] Token to be unquoted.
//
// [Returns]
//   (StringView): Unquoted token.
//
{   // {{{

    // Return the token as-is if it is empty or does not start with a quote.
    if (token.empty() or ((token[0] != '\'') and (token[0] != '"')))
        return token;

    // Remove the surrounding quotes if the token starts with a quote.
    const char q = token[0];
    token.remove_prefix(1);

    // Remove the closing quote if it exists.
    if ((not token.empty()) and (token.back() == q))
        token.remove_suffix(1);

    return token;

}   // }}}

String shell_quote(StringView sv, bool close)
// Quote the string with single quotes if it contains shell-special characters.
// The closing quote is omitted if "close" is false, so that users can continue typing.
//
// [Args]
//   sv    (StringView): [IN] String to be quoted.
//   close (bool)      : [IN] If true, add a closing quote
//
{   // {{{

    constexpr auto is_safe = [](unsigned char c) -> bool
    // Check if the character is safe for shell quoting.
    //
    // [Args]
    //   c (unsigned char): [IN] Character to be checked.
    //
    // [Returns]
    //   (bool): True if the character is safe for shell quoting.
    {
        constexpr StringView safe = "_-./:@+=,%~";
        return std::isalnum(c) or (c >= 0x80) or (safe.find(c) != StringView::npos);
    };

    // Keep "~/" outside of the quotes, otherwise the tilde expansion is disabled.
    if (sv.starts_with("~/") and (sv.size() > 2))
        return "~/" + shell_quote(sv.substr(2), close);

    // Return as it is if all characters are safe (non-ASCII characters are regarded as safe).
    if (std::all_of(sv.begin(), sv.end(), is_safe)) return String(sv);

    // Quote the string with single quotes, and escape any single quotes inside the string.
    String r = "'";
    for (const char c : sv)
    {
        if (c == '\'') { r += "'\\''"; }
        else           { r += c;       }
    }

    // Add a closing quote if requested.
    if (close) r += '\'';

    return r;

}   // }}}

// vim: expandtab tabstop=4 shiftwidth=4 fdm=marker
