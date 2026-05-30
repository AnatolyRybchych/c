#ifndef MC_DATA_ENCODING_URL_H
#define MC_DATA_ENCODING_URL_H

#include <mc/data/str.h>
#include <mc/error.h>

/// @brief decodes url-encoded sequence; the output does not include '\0'
/// @param encoded source string to decode
/// @param bufsz size of the buf
/// @param buf char[bufsz]
/// @param decoded_size amount of bytes written to buf
/// @return remaining unencoded data which can occur in case of insufficient bufsz or unfinished escape sequecnce %XX
MC_Str mc_urldecode(MC_Str encoded, size_t bufsz, char buf[], size_t *decoded_size);

/// @brief encodes byte sequence; the output does not include '\0'
/// @param decoded source bytes to encode
/// @param bufsz size of the buf
/// @param buf char[bufsz]
/// @param encoded_size amount of bytes written to buf
/// @return remaining encoded data which can occur in case of insufficient bufsz
MC_Str mc_urlencode(MC_Str decoded, size_t bufsz, char buf[], size_t *encoded_size);

#endif // MC_DATA_ENCODING_URL_H
