#ifndef MC_IO_FILE_H
#define MC_IO_FILE_H

#include <mc/data/stream.h>
#include <mc/data/str.h>

#define MC_STDIN mc_get_stdin()
#define MC_STDOUT mc_get_stdout()
#define MC_STDERR mc_get_stderr()

typedef unsigned MC_OpenMode;
enum MC_OpenMode{
    // Open the file to read
    MC_OPEN_READ = 1 << 0,

    // Open the file to write
    MC_OPEN_WRITE = 1 << 1,

    // Open file in non-blocking mode if its possible
    MC_OPEN_ASYNC = 1 << 2,

    // Open clear the file if its not empty
    MC_OPEN_CLEAR = 1 << 3,

    // Open with the cursor at the end of the file
    MC_OPEN_END = 1 << 4,

    // Do not open the file if it does not exists
    MC_OPEN_EXISTING = 1 << 5,

    // Do not open the file if it is already exists
    MC_OPEN_NEW = 1 << 5,

    // Create file if it does not exists
    MC_OPEN_CREATE = 1 << 6,
};

MC_Error mc_fopen(MC_Stream **file, MC_Str path, MC_OpenMode mode);

MC_Stream *mc_get_stdin(void);
MC_Stream *mc_get_stdout(void);
MC_Stream *mc_get_stderr(void);

#endif // MC_IO_FILE_H
