#ifndef MC_NET_PROTO_HTTP_READER_H
#define MC_NET_PROTO_HTTP_READER_H

#include <mc/net/proto/http/message.h>
#include <mc/data/stream.h>

typedef struct MC_HttpReader MC_HttpReader;

MC_Error mc_http_reader_new(MC_Alloc *alloc, MC_HttpReader **reader, MC_Stream *source);
void mc_http_reader_delete(MC_HttpReader *reader);
MC_Error mc_http_reader_read(MC_HttpReader *reader, MC_HttpMessage *message, MC_Stream *body);
MC_Error mc_http_reader_finish(MC_HttpReader *reader);

#endif // MC_NET_PROTO_HTTP_READER_H
