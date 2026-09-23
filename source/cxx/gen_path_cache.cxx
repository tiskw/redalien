////////////////////////////////////////////////////////////////////////////////////////////////////
/// C++ source file: gen_path_cache.cxx                                                          ///
////////////////////////////////////////////////////////////////////////////////////////////////////

// Include the primary header.
#include "gen_path_cache.hxx"

// Include STL headers.
#include <iostream>

// Include the headers of custom modules.
#include "utils.hxx"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Public functions
////////////////////////////////////////////////////////////////////////////////////////////////////

int32_t generate_path_commands_cache(void)
// Print all commands in PATH environment variable.
//
// [Returns]
//   (int): EXIT_SUCCESS if no error occurred.
//
{   // {{{

    // Get the "PATH" environment variable, and return failure if it is not set.
    const char* env_path = std::getenv("PATH");
    if (env_path == nullptr)
        return EXIT_FAILURE;

    // Initialize the output vector.
    Vector<String> result;

    // Initialize a set of searched path.
    StringSet searched_path;

    // Search all directories in "PATH" and get all executable files.
    for (const StringView path : split(env_path, ":"))
    {
        // Skip if the path in "PATH" is not a directory, or already searched.
        if ((not stdfs::is_directory(path)) or searched_path.contains(path))
            continue;

        // Append all target files in the directory.
        // Skip if the entry is not a regular file or symbolic link.
        for (const stdfs::directory_entry& entry : stdfs::directory_iterator(path, stdfs::directory_options::skip_permission_denied))
        {
            // Skip if neither a regular file nor a symboic link.
            if ((not entry.is_regular_file()) and (not entry.is_symlink()))
                continue;

            // Get file extension with lower case.
            String ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), [](char c){return std::tolower(static_cast<unsigned char>(c));});

            // Skip files of specified extensions.
            if ((ext == ".dat" ) or (ext == ".dll") or (ext == ".exe") or (ext == ".ini" ) or (ext == ".jpg")
             or (ext == ".json") or (ext == ".log") or (ext == ".mof") or (ext == ".nls" ) or (ext == ".so" )
             or (ext == ".png" ) or (ext == ".txt") or (ext == ".xml") or (ext == ".yaml"))
                continue;

            // Skip files of specified extensions.
            static const StringSet exts_exclude = {".dat", ".dll", ".exe", ".ini", ".jpg", ".json", ".log", ".mof", ".nls", ".so", ".png", ".txt",  ".xml", ".yaml"};
            if (exts_exclude.contains(ext))
                continue;

            // Notes: Ideally, we would like to extract executable files here, but doing it would significantly
            //        slow down the processing speed (on WSL2), so we are only checking the file types and extensions.

            // Add the file name to the output vector.
            result.emplace_back(entry.path().filename().c_str());
        }

        // Mark the path as searched.
        searched_path.emplace(path);
    }

    // Sort and remove duplicated command names.
    deduplicate(result);

    // Dump the deduplicated result.
    for (const String& name : result)
        std::cout << name << "\n";

    return EXIT_SUCCESS;

}   // }}}

// vim: expandtab tabstop=4 shiftwidth=4 fdm=marker
