ifeq ($(OS),Windows_NT)
    CC := gcc
    LD := gcc
    AR := ar

    mkdir_p = if not exist "$(subst /,\,$1)" mkdir "$(subst /,\,$1)"
    rm_rf   = $(foreach f,$1,rmdir /s /q "$(subst /,\,$f)" 2>nul & del /f /q "$(subst /,\,$f)" 2>nul & )
    cp_file = copy /y "$(subst /,\,$1)" "$(subst /,\,$2)" >nul
    cp_dir  = xcopy /e /i /y /q "$(subst /,\,$1)" "$(subst /,\,$2)" >nul
    touch_file = type nul > "$(subst /,\,$1)"

    executable  = $1.exe
    loadable    = $1.dll
    lua_link    = -llua54
    wm_backend  = win32_wm
    wm_libs     = -lgdi32 -luser32
    demo_skip   = http-server
    subprojects = demo
else
    CC := cc
    LD := cc
    AR := ar

    mkdir_p = mkdir -p $1
    rm_rf   = rm -rf $1
    cp_file = cp -p $1 $2
    cp_dir  = cp -rp $1/. $2
    touch_file = touch $1

    executable  = $1
    loadable    = $1.so
    lua_link    =
    wm_backend  = xlib_wm
    wm_libs     = -lX11 -lm
    demo_skip   =
    subprojects = demo mutation
endif

rwildcard = $(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))
