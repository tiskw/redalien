////////////////////////////////////////////////////////////////////////////////////////////////////
/// C++ header file: path_x.hxx                                                                  ///
///                                                                                              ///
/// This file defines the class `PathX` that provides utility functions related to file path.    ///
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef PATH_X_HXX
#define PATH_X_HXX

// Include STL headers.
#include <chrono>
#include <filesystem>

// Include the headers of custom modules.
#include "dtypes.hxx"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Data types
////////////////////////////////////////////////////////////////////////////////////////////////////

struct DirEntry
// An entry of the directory listing.
{
    String name;     // Entry name (a trailing '/' is appended for directories).
    bool   is_dir;   // True if the entry is a directory (symlinks are followed).
    bool   is_exec;  // True if the entry is a non-directory file executable by the owner.
};

struct ListdirResult
// Result of PathX::listdir.
{
    Vector<DirEntry> entries;            // Matched entries (directories first, then files).
    bool             truncated = false;  // True if the listing was stopped before the end.
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Class definitions
////////////////////////////////////////////////////////////////////////////////////////////////////

class PathX : public Path
{
    public:

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Constructors and destructors
        ////////////////////////////////////////////////////////////////////////////////////////////

        explicit PathX(const std::filesystem::path& path);
        // Constructor of PathX.
        //
        // [Args]
        //   path (const std::filesystem::path&): [IN] Source path to be copied.

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Member functions
        ////////////////////////////////////////////////////////////////////////////////////////////

        ListdirResult listdir(StringView prefix = "") const;
        // Returns the entries in this directory whose names start with the given prefix.
        // Hidden entries are listed only if the prefix starts with a dot.
        //
        // [Args]
        //   prefix (StringView): [IN] Prefix of the entry names to be listed.
        //
        // [Returns]
        //   (ListDirResult): Matched entries and the truncation flag.

        // Vector<String> listdir(uint32_t n_max_items = 128) const;
        // Returns a list of names of the entries in the given directory path.
        // The list is sorted in ascending order.
        //
        // [Args]
        //   n_max_items (uint32_t): [IN] The maximum number of items to be listed.
        //
        // [Returns]
        //   (Vector<string>): List of names of the entries.
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Public functions
////////////////////////////////////////////////////////////////////////////////////////////////////

Tuple<PathX, String> split_to_target_and_query(const Vector<StringView>& tokens);
// Split the path to completion target and query.
//
// [Args]
//   tokens (const Vector<StringView>&): [IN] User input tokens.
//
// [Returns]
//   (PathX) : Completion target.
//   (String): Completion query.
//

#endif

// vim: expandtab tabstop=4 shiftwidth=4 fdm=marker
