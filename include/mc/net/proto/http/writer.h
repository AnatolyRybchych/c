#ifndef MC_NET_PROTO_HTTP_WRITER_H
#define MC_NET_PROTO_HTTP_WRITER_H

#include <mc/net/proto/http/message.h>
#include <mc/data/stream.h>

typedef struct MC_HttpWriter MC_HttpWriter;

MC_Error mc_http_writer_new(MC_Alloc *alloc, MC_HttpWriter **writer);
void mc_http_writer_delete(MC_HttpWriter *writer);
MC_Error mc_http_writer_beg(MC_HttpWriter *writer, MC_Stream *destination, const MC_HttpMessage *message);
MC_Error mc_http_writer_write(MC_HttpWriter *writer);

#endif // MC_NET_PROTO_HTTP_WRITER_H
