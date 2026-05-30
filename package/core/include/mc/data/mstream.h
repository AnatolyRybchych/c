#ifndef MC_DATA_MSTREAM_H
#define MC_DATA_MSTREAM_H

#include <mc/data/stream.h>

MC_Error mc_mstream(MC_Alloc *alloc, MC_Stream **mstream);
size_t mc_mstream_size(MC_Stream *mstream);
void mc_mstream_truncate(MC_Stream *mstream);

#endif // MC_DATA_MSTREAM_H
