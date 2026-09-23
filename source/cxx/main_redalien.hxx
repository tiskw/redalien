////////////////////////////////////////////////////////////////////////////////////////////////////
/// C++ header file: main_redalien.hxx                                                           ///
///                                                                                              ///
/// Main function of RedAlien.                                                                   ///
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef MAIN_REDALIEN_HXX
#define MAIN_REDALIEN_HXX

// Include STL headers.
#include <cstdint>

////////////////////////////////////////////////////////////////////////////////////////////////////
// Constants
////////////////////////////////////////////////////////////////////////////////////////////////////

// Version information, software name, and description of RedAlien.
inline constexpr const char* VERSION = "2026.09.22";
inline constexpr const char* SOFTWARE_NAME = "redalien";
inline constexpr const char* SOFTWARE_DESC = "A next-generation command line editor for Bash";

////////////////////////////////////////////////////////////////////////////////////////////////////
// Function definitioins
////////////////////////////////////////////////////////////////////////////////////////////////////

int32_t main_redalien(int32_t argc, char* argv[], const char* input_ptr);
// Actual main function of RedAlien.
//
// [Args]
//   argc      (int)         : [IN] The number of command line arguments.
//   argv      (char*[])     : [IN] The array of command line arguments.
//   input_ptr (const char*) : [IN] User input (for debugging).
//
// [Returns]
//   (int32_t): Return code.

#endif

// vim: expandtab tabstop=4 shiftwidth=4 fdm=marker
