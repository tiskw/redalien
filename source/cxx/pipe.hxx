////////////////////////////////////////////////////////////////////////////////////////////////////
/// C++ header file: pipe.hxx                                                                    ///
///                                                                                              ///
/// This file defines the class `Pipe` which is a RAII wrapper of a pipe.                        ///
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef PIPE_HXX
#define PIPE_HXX

////////////////////////////////////////////////////////////////////////////////////////////////////
// Class definition
////////////////////////////////////////////////////////////////////////////////////////////////////

class Pipe
// RAII wrapper of a pipe.
// Both ends are closed automatically on destruction.
{
    public:

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Constructors
        ////////////////////////////////////////////////////////////////////////////////////////////

         Pipe(void);
        ~Pipe(void);
        // Constructor and destructor of Pipe.

        // NOTE: This class should be non-copyable and non-movable,
        // because this class owns raw file descriptors.
        Pipe(const Pipe&)            = delete;
        Pipe& operator=(const Pipe&) = delete;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Member functions
        ////////////////////////////////////////////////////////////////////////////////////////////

        void close_r(void) noexcept;
        void close_w(void) noexcept;
        // Close the read/write end of the pipe.

        int fd_r(void) const noexcept;
        int fd_w(void) const noexcept;
        // Get the read/write end of the pipe.
        //
        // [Returns]
        //   (int): Read/write end of the pipe.

        bool open(void) noexcept;
        // Open a pipe with O_CLOEXEC flag.
        //
        // [Returns]
        //   (bool): True if the pipe is opened successfully, false otherwise.

        void set_read_nonblock(void) noexcept;
        // Set the read end of the pipe to non-blocking mode.

    private:

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Private member variables
        ////////////////////////////////////////////////////////////////////////////////////////////

        int fds[2];
        // File descriptors for the read and write ends of the pipe.

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Private member functions
        ////////////////////////////////////////////////////////////////////////////////////////////

        static void close_fd(int& fd) noexcept;
        // Close the given file descriptor and set it to -1.
        //
        // [Args]
        //   fd (int&): [IN/OUT] File descriptor to be closed.
};

#endif

// vim: expandtab tabstop=4 shiftwidth=4 fdm=marker
