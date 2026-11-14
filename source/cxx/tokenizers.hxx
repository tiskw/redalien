////////////////////////////////////////////////////////////////////////////////////////////////////
/// C++ header file: tokenizers.hxx                                                              ///
///                                                                                              ///
/// Functions to tokenize a string and quote/unquote a token.                                    ///
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef TOKENIZERS_HXX
#define TOKENIZERS_HXX

// Include the headers of custom modules.
#include "dtypes.hxx"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Data types
////////////////////////////////////////////////////////////////////////////////////////////////////

enum TokenizeOption : uint8_t
{
    TOKENIZE_PLAIN   = 0,       // Plain tokenization without any special options.
    TOKENIZE_KEEP_WS = 1 << 0,  // Keep white-space tokens in the result.
    TOKENIZE_DEQUOTE = 1 << 1,  // Strip single/double quotes from tokens.
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Public functions
////////////////////////////////////////////////////////////////////////////////////////////////////

Generator<StringView> tokenize(StringView sv, TokenizeOption option = TOKENIZE_PLAIN);
// Split the given string to tokens.
//
// [Args]
//   sv     (StringView)    : [IN] Target string to be tokenized.
//   option (TokenizeOption): [IN] Tokenization options.
//
// [Returns]
//   (Generator<StringView>): Tokenized string views.

Generator<String> tokenize_with_placeholder_replacement(StringView sv, const StringMap& extra, TokenizeOption option = TOKENIZE_PLAIN);
// Tokenize the given command string with placeholder replacement.
//
// [Args]
//   sv     (const String&)   : [IN] Target string to be tokenize.
//   extra  (const StringMap&): [IN] Extra placeholder values.
//   option (TokenizeOption)  : [IN] Tokenization options.
//
// [Returns]
//   (Generator<String>): Tokenized string views with placeholder replacement.

bool is_open_quote(StringView token) noexcept;
// Returns true if the token starts with a quote that is not closed yet (e.g. "'my fi").
//
// [Args]
//   token (StringView): [IN] Token to be checked.
//
// [Returns]
//   (bool): True if the token starts with a quote that is not closed yet.

String shell_quote(StringView sv, bool close = true);
// Quote the string with single quotes if it contains shell-special characters.
// The closing quote is omitted if "close" is false, so that users can continue typing.
//
// [Args]
//   sv    (StringView): [IN] String to be quoted.
//   close (bool)      : [IN] If true, add a closing quote.
//
// [Returns]
//   (String): Quoted string.

StringView unquote(StringView token) noexcept;
// Strip the surrounding quotes of a token ("'my fi" -> "my fi", "'a b'" -> "a b").
// A token that does not start with a quote is returned as-is.
//
// [Args]
//   token (StringView): [IN] Token to be unquoted.
//
// [Returns]
//   (StringView): Unquoted token.

#endif

// vim: expandtab tabstop=4 shiftwidth=4 fdm=marker
