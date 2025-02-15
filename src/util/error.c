#include <mc/util/error.h>

#include <errno.h>
#include <string.h>

MC_Error mc_error_from_errno(int err_no){
    switch (err_no){
    case 0: return MCE_OK;
    case ENOENT: return MCE_NOT_FOUND;
    case EBUSY: return MCE_BUSY;
    case EINVAL: return MCE_INVALID_INPUT;
    case EAGAIN: return MCE_AGAIN;
    case ECONNREFUSED: return MCE_CONNECTION_REFUSED;
    case EPERM: return MCE_NOT_PERMITTED;
    case EACCES: return MCE_NOT_PERMITTED;
    case EADDRINUSE: return MCE_ADDRESS_IN_USE;
    default:
        mc_fmt(MC_STDERR, "mc_error_from_errno(%i: %s) -> MCE_UNKNOWN\n", errno, strerror(errno));
        return MCE_UNKNOWN;
    }
}
