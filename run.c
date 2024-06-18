#include <stdio.h>

#include <mc.h>
#include <mc/dlib.h>

int main(){
    MC_DLib *gl;
    MC_Error error = mc_dlib_open(&gl, MC_STRC("libGL.so"));
    if(error){
        printf("mc_dlib_open: %s\n", mc_strerror(error));
        return 1;
    }

    void *func = mc_dlib_get(gl, MC_STRC("glClear"));
    void (*glClear)(int bit) = *(void (**)(int bit))&func;
    glClear(0);

    mc_dlib_close(gl);
    return 0;
}
