////////////////////////////////////////////////////////////////////////////////////////////////////
/// C++ source file: path_x.cxx                                                                  ///
////////////////////////////////////////////////////////////////////////////////////////////////////

// Include the primary header.
#include "path_x.hxx"

// Include the headers of custom modules.
#include "utils.hxx"

////////////////////////////////////////////////////////////////////////////////////////////////////
// File-local functions
////////////////////////////////////////////////////////////////////////////////////////////////////

// Unnamed namespace for making classes and functions file-local.
namespace
{
    constexpr std::chrono::milliseconds listdir_timeout = std::chrono::milliseconds(50);
    // Timeout for the directory scan (protection against extremely slow file systems).

    constexpr uint32_t listdir_n_max_matches = 256;
    // Maximum number of matched entries to be returned (protection against extremely large directories).
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PathX: Constructors and destructors
////////////////////////////////////////////////////////////////////////////////////////////////////

PathX::PathX(const stdfs::path& path) : stdfs::path(path)
{ /* Do nothing, initializer lists only. */ }

////////////////////////////////////////////////////////////////////////////////////////////////////
// PathX: Member functions
////////////////////////////////////////////////////////////////////////////////////////////////////

ListdirResult PathX::listdir(StringView prefix) const
{   // {{{

    // Initialize the returned value.
    ListdirResult result;

    // Empty path will be regarded as the current directory, and "~" is expanded to the home directory.
    const PathX target = PathX(expand_tilde(((not this->empty()) ? *this : PathX("./")).string()));

    // Returns empty result if the target directory does not exist.
    std::error_code ec;
    if ((not stdfs::is_directory(target, ec)) or ec)
        return result;

    // Hidden entries are shown only if the prefix starts with a dot.
    const bool show_dot = prefix.starts_with('.');

    // Compute the deadline of the directory scan.
    const auto deadline = std::chrono::steady_clock::now() + listdir_timeout;

    // Open the directory iterator (non-throwing version).
    stdfs::directory_iterator iter(target, stdfs::directory_options::skip_permission_denied, ec);
    if (ec) return result;

    for (; iter != stdfs::directory_iterator(); iter.increment(ec))
    {
        // Stop on iteration errors.
        if (ec) break;

        // Stop if the deadline has passed (protection against extremely slow file systems).
        if (std::chrono::steady_clock::now() >= deadline)
        { result.truncated = true; break; }

        // Get the entry name. Note that "filename()" does not access the file system.
        const stdfs::directory_entry& entry = *iter;
        String name = entry.path().filename().string();

        // Filter by the prefix BEFORE any costly operation (no stat is issued for unmatched entries).
        if ((not show_dot) and name.starts_with('.')) continue;
        if (not name.starts_with(prefix))            continue;

        // Stop if the number of matched entries exceeds the limit.
        if (result.entries.size() >= listdir_n_max_matches)
        { result.truncated = true; break; }

        // Determine the entry type. libstdc++ uses the cached d_type when available,
        // therefore stat is issued only for symlinks and DT_UNKNOWN entries.
        std::error_code ec_type;
        const bool is_dir = entry.is_directory(ec_type) and (not ec_type);

        // Check the executable permission (requires stat, but only for matched non-directories).
        bool is_exec = false;
        if (not is_dir)
        {
            std::error_code ec_stat;
            const stdfs::file_status status = entry.status(ec_stat);
            is_exec = (not ec_stat) and ((status.permissions() & stdfs::perms::owner_exec) != stdfs::perms::none);
        }

        // Append a slash to the directory name.
        if (is_dir) name += '/';

        result.entries.push_back({std::move(name), is_dir, is_exec});
    }

    // Sort the entries in the same way as `ls --group-directories-first`.
    std::sort(result.entries.begin(), result.entries.end(), [](const DirEntry& e1, const DirEntry& e2) noexcept -> bool
    {
        if (e1.is_dir != e2.is_dir) return e1.is_dir;
        return e1.name < e2.name;
    });

    return result;

}   // }}}

// Vector<String> PathX::listdir(uint32_t n_max_items) const
// {   // {{{
// 
//     // Initilize returned vector.
//     Vector<String> result;
// 
//     // Empty path will be regarded as a current directory.
//     PathX target = (not this->empty()) ? *this : PathX("./");
// 
//     // Replace "~" to home directory.
//     if (not target.empty())
//         target = PathX(expand_tilde(target.string()));
// 
//     // Returns empty vector if the target directory does not exist.
//     if (not stdfs::is_directory(target))
//         return result;
// 
//     // Collect all files and directories inside the target directory.
//     std::error_code ec;
//     for (const auto& entry : stdfs::directory_iterator(target, stdfs::directory_options::skip_permission_denied))
//     {
//         // Get relative path.
//         const auto path_relative = entry.path().lexically_relative(target);
// 
//         //
//         bool is_dir = entry.is_directory(ec);
//         if (ec) is_dir = false;
// 
//         // Add to the returned vector.
//         result.push_back(path_relative.string() + (is_dir ? "/" : ""));
// 
//         // Stop directory search if exceeds the maximum number of items.
//         // Because it takes too long time for searching a big directory and
//         // it tend to prevent user's comfortable command editing experience.
//         if (result.size() >= n_max_items)
//             break;
//     }
// 
//     // Define sorting key function.
//     // The sorting rule is the same as `ls --group-directories-first` command.
//     constexpr auto sort_key_func = [](const String& s1, const String& s2) noexcept -> bool
//     {
//         const bool is_dir1 = (s1.size() > 0) and (s1.back() == '/');
//         const bool is_dir2 = (s2.size() > 0) and (s2.back() == '/');
// 
//         if      (is_dir1 and is_dir2) return (s1 < s2);
//         else if (is_dir1            ) return true;
//         else if (            is_dir2) return false;
//         else                          return (s1 < s2);
//     };
// 
//     // Sort list results.
//     std::sort(result.begin(), result.end(), sort_key_func);
// 
//     return result;
// 
// }   // }}}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Public functions
////////////////////////////////////////////////////////////////////////////////////////////////////

Tuple<PathX, String> split_to_target_and_query(const Vector<StringView>& tokens)
{   // {{{

    // Initialize the target path.
    PathX basepath = PathX("");

    // Update the base path if the token is not empty.
    if (tokens.size() > 0)
        basepath = PathX(strip(tokens.back()));

    // Split the target token to parent path and file name.
    return {PathX(basepath.parent_path()), basepath.filename().string()};

}   // }}}

// vim: expandtab tabstop=4 shiftwidth=4 fdm=marker
