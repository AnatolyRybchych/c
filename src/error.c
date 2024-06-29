#include <mc/error.h>

#include <errno.h>

extern inline const char *mc_strerror(MC_Error err);

MC_Error mc_error_from_errno(int err_no){
    switch (err_no){
    case 0: return MCE_OK;
    case ENOENT: return MCE_NOT_FOUND;
    case EBUSY: return MCE_BUSY;
    case EINVAL: return MCE_INVALID_INPUT;
    case EAGAIN: return MCE_AGAIN;
    default: return MCE_NOT_IMPLEMENTED;
    }
}
