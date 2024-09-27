#include <mc/dlib.h>
#include <mc/data/string.h>
#include <dlfcn.h>

#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

struct MC_DLib{
#ifdef __linux__
    // TODO: use mmap directly to avoid extra dependencies
    void *dl;
#endif

    unsigned path_len;
    char path[];
};

MC_Error mc_dlib_open(MC_DLib **lib, MC_Str path){
    MC_DLib *new;
    MC_RETURN_ERROR(mc_alloc(NULL, sizeof(MC_DLib) + sizeof(char[MC_STR_LEN(path) + 1]), (void**)&new));

    memcpy(new->path, path.beg, new->path_len = MC_STR_LEN(path));
    new->path[new->path_len] = '\0';

    new->dl = dlopen(new->path, RTLD_LAZY);
    if(!new->dl){
        mc_dlib_close(new);
        return MCE_NOT_FOUND;
    }

    *lib = new;

    return MCE_OK;
}

void mc_dlib_close(MC_DLib *lib){
    if(!lib){
        return;
    }
#ifdef __linux__
    if(lib->dl){
        dlclose(lib->dl);
    }
#endif

    mc_free(NULL, lib);
}

void *mc_dlib_get(MC_DLib *lib, MC_Str symbol){
    void *res = NULL;

#ifdef __linux__
    //TODO: avoid allocation
    MC_String *sym;
    mc_string(NULL, &sym, symbol);
    assert(sym != NULL);

    res = dlsym(lib->dl, sym->data);
    mc_free(NULL, sym);
#endif

    return res;
}

MC_Str mc_dlib_path(const MC_DLib *lib){
    return MC_STR(lib->path, lib->path + lib->path_len);
}
