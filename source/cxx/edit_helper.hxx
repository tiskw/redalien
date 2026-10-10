////////////////////////////////////////////////////////////////////////////////////////////////////
/// C++ header file: edit_helper.hxx                                                             ///
///                                                                                              ///
/// This file defines the class `EditHelper` which will be used for computing command/path       ///
/// completion candidates, and present them to users.                                            ///
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef EDIT_HELPER_HXX
#define EDIT_HELPER_HXX

// Include STL headers.
#include <future>

// Include the headers of custom modules.
#include "carapace_service.hxx"
#include "config.hxx"
#include "dtypes.hxx"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Class definitions
////////////////////////////////////////////////////////////////////////////////////////////////////

class EditHelper
{
    public:

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Constructors and destructors
        ////////////////////////////////////////////////////////////////////////////////////////////

        EditHelper(uint16_t rows, uint16_t cols, const Path& outdir, const RedAlienConfig& cfg);
        // Default constructor of EditHelper.
        //
        // [Args]
        //   rows   (uint16_t)             : [IN] Height of completion area.
        //   cols   (uint16_t)             : [IN] Width of completion area.
        //   outdir (const Path&)          : [IN] Path to output directory.
        //   cfg    (const RedAlienConfig&): [IN] Config data.

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Member functions
        ////////////////////////////////////////////////////////////////////////////////////////////

        const Vector<String>& candidate(StringView lhs);
        // Returns candidates of completion.
        //
        // [Args]
        //   lhs (StringView): [IN] Left-hand-side of the user input.
        //
        // [Returns]
        //   (Vector<String>): Array of lines (strings) for showing completion candidates to users.

        String complete(StringView lhs) const;
        // Execute completion.
        // This function should be called after calling `candidate` function.
        //
        // [Args]
        //   lhs (StringView): [IN] Left-hand-side of the user input.
        //
        // [Returns]
        //   (String): Completed left-hand-side string.

    private:

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Member variables
        ////////////////////////////////////////////////////////////////////////////////////////////

        Size area_size;
        // Size of the drawing area.

        CarapaceService carapace_service;
        // An instance of CarapaceService for computing completion candidates from "carapace".

        Vector<String> cache_commands;
        // Cache of available command names.

        CandCacheMap cache_cands_lhs;
        // Cache of completion candidates, where the key is the hash value of the left-hand-side
        // string of the completion target.

        Vector<Pair<String, String>>* cands = nullptr;
        // Completion candidates. This is a pointer to the instance on cache_cands_lhs.

        Vector<String>* lines = nullptr;
        // Completion lines. This is a pointer to the instance on cache_cands_lhs.

        CandCacheEntry none_cache_entry;
        // Candidate cache entry for NONE case.

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Constant member variables (e.g. copied values from the config data)
        ////////////////////////////////////////////////////////////////////////////////////////////

        const uint16_t column_padding;
        // Margin of column display in the completion list.

        const Vector<Completion> completions;
        // Completion patterns and their types and optional strings.

        const StringMap previews;
        // Collection of MIME type and its preview command.

        const String preview_delim;
        // Delimiter of the preview window.

        const float preview_ratio;
        // Width of the preview window.

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Private functions
        ////////////////////////////////////////////////////////////////////////////////////////////

        void cands_command(const Vector<StringView>& tokens, const String& option);
        // Compute command completion candidates.
        // The result candidates will be stored in `this->cands`.
        //
        // [Args]
        //   tokens (const Vector<StringView>): [IN] Parsed tokens of the user input.
        //   option (const String&)           : [IN] Optional string of the completion.

        void cands_carapace(const Vector<StringView>& tokens);
        // Compute completion candidates from carapace.
        //
        // [Args]
        //   tokens (const Vector<StringView>): [IN] Parsed tokens of the user input.

        void cands_filepath(const Vector<StringView>& tokens);
        // Compute file path completion candidates.
        // The result candidates will be stored in `this->cands`.
        //
        // [Args]
        //   tokens (const Vector<StringView>): [IN] Parsed tokens of the user input.

        void cands_grep(const Vector<StringView>& tokens, const String& option);
        // Compute command option candidates from grep.
        //
        // [Args]
        //   tokens (const Vector<StringView>): [IN] Parsed tokens of the user input.
        //   option (const String&)           : [IN] Optional string (tab-separated string).

        void cands_preview(const Vector<StringView>& tokens);
        // Compute completion candidates for preview.
        //
        // [Args]
        //   tokens (const Vector<StringView>): [IN] Parsed tokens of the user input.

        void cands_shell(const Vector<StringView>& tokens, const String& option);
        // Compute completion candidates from shell command.
        //
        // [Args]
        //   tokens (const Vector<StringView>): [IN] Parsed tokens of the user input.
        //   option (const String&)           : [IN] Optional string (normally it is a shell command).

        void lines_from_cands(const Vector<Pair<String, String>>& cands);
        // Convert candidate to strings that will be shown to users.
        //
        // [Args]
        //   cands (const Vector<Pair<String, String>>&): [IN] Completion candidates.

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Static private functions
        ////////////////////////////////////////////////////////////////////////////////////////////

        static void init_shared_futures(const Path& outdir, const RedAlienConfig& cfg);
        // Initialize shared future instances for caching command names and compiled regular expression patterns.
        //
        // [Args]
        //   outdir (const Path&)          : [IN] Path to output directory.
        //   cfg    (const RedAlienConfig&): [IN] Config data for initializing the shared futures.
};

#endif

// vim: expandtab tabstop=4 shiftwidth=4 fdm=marker
