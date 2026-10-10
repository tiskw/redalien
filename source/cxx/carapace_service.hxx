////////////////////////////////////////////////////////////////////////////////////////////////////
/// C++ header file: carapace_service.hxx                                                        ///
///                                                                                              ///
/// A class that provides shell-style tab completion by communicating with "carapace" process    ///
/// running in the background.                                                                   ///
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CARAPACE_SERVICE_HXX
#define CARAPACE_SERVICE_HXX

// Include the headers of custom modules.
#include "config.hxx"
#include "dtypes.hxx"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Class definition
////////////////////////////////////////////////////////////////////////////////////////////////////

class CarapaceService
{
    public:

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Constructors and destructors
        ////////////////////////////////////////////////////////////////////////////////////////////

        explicit CarapaceService(const RedAlienConfig& cfg);
        // Constructor for the CarapaceService class.

        ~CarapaceService(void);
        // Destructor for the CarapaceService class.

        // NOTE: This class should be non-copyable and non-movable.
        CarapaceService(const CarapaceService&)              = delete;
        CarapaceService& operator = (const CarapaceService&) = delete;
        CarapaceService(CarapaceService&&)                   = delete;
        CarapaceService& operator = (CarapaceService&&)      = delete;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Member functions
        ////////////////////////////////////////////////////////////////////////////////////////////

        Generator<Tuple<StringView, StringView, const char*>> complete(const Vector<StringView>& tokens);
        // Returns a list of completion candidates for the given command-line string.
        //
        // [Args]
        //   tokens (const Vector<StringView>&): [IN] The parsed tokens of the user input.
        //
        // [Returns]
        //   (Generator<Tuple<StringView, StringView, const char*>>): A generator that yields tuples of (candidate, description, color_code).

    private:

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Private member variables
        ////////////////////////////////////////////////////////////////////////////////////////////

        Map<uint64_t, Vector<Tuple<String, String, const char*>>> cache;
        // Cache for storing previously computed completion results.

        Path path_carapace_bin;
        // Path to the "carapace" binary executable.

        const StringMap& colors;
        // Reference to the color configuration map.

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Private member functions
        ////////////////////////////////////////////////////////////////////////////////////////////

        const char* get_color(StringView style) const;
        // Returns the color code for the given style string.
        //
        // [Args]
        //   style (StringView): [IN] The style string returned from the "carapace".
        //
        // [Returns]
        //   (const char*): The color code corresponding to the given style string.
};

#endif

// vim: expandtab tabstop=4 shiftwidth=4 fdm=marker
