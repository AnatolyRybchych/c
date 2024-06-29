#ifndef MC_IO_FILE_H
#define MC_IO_FILE_H

#include <mc/io/stream.h>
#include <mc/data/str.h>

#define MC_STDIN mc_get_stdin()
#define MC_STDOUT mc_get_stdout()
#define MC_STDERR mc_get_stderr()

typedef unsigned MC_OpenMode;
enum MC_OpenMode{
    MC_OPEN_READ,
    MC_OPEN_WRITE,
    MC_OPEN_APPEND,
};

MC_Error mc_file_open(MC_Stream **file, MC_Str path, MC_OpenMode mode);

MC_Stream *mc_get_stdin(void);
MC_Stream *mc_get_stdout(void);
MC_Stream *mc_get_stderr(void);

#endif // MC_IO_FILE_H
