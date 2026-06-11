NAME := wm-lua

PREFIX  ?= /usr/local
DESTDIR ?=
CC      ?= gcc
AR      ?= ar
CFLAGS  ?= -ggdb -Wall -Wextra -MMD -MP

# dependency headers (lua + mc) come from the (staging or real) install prefix
INCLUDE := -Iinclude -I$(DESTDIR)$(PREFIX)/include

SRC_DIR := src
OBJ_DIR := build/obj
LIB     := lib$(NAME).a

include ../../rules.mk

MODULE      := $(call loadable,mcwm)
MODULE_SRC  := module/$(call wm_backend).c
MODULE_LIBS := -L$(DESTDIR)$(PREFIX)/lib -l$(call wm_backend) -lwm -lgraphics -los -lnet -lgeometry -lcore $(call wm_libs) $(lua_link)

sources := $(filter-out $(addprefix $(SRC_DIR)/,$(EXCLUDE)),$(call rwildcard,$(SRC_DIR),*.c))
objects := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(sources))
deps    := $(objects:.o=.d)

all: $(LIB) $(MODULE)

$(LIB): $(objects)
	$(AR) rcs $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@$(call mkdir_p,$(@D))
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<

$(MODULE): $(LIB) $(MODULE_SRC)
	$(CC) -shared $(CFLAGS) $(INCLUDE) -o $@ $(MODULE_SRC) -Wl,--start-group $(LIB) $(MODULE_LIBS) -Wl,--end-group -Wl,--export-all-symbols

install: all
	$(call mkdir_p,$(DESTDIR)$(PREFIX)/lib)
	$(call mkdir_p,$(DESTDIR)$(PREFIX)/include/mc)
	$(call cp_file,$(LIB),$(DESTDIR)$(PREFIX)/lib)
	$(call cp_file,$(MODULE),$(DESTDIR)$(PREFIX)/lib)
	$(call cp_dir,include/mc,$(DESTDIR)$(PREFIX)/include/mc)

clean:
	-$(call rm_rf,$(OBJ_DIR) $(LIB) $(MODULE))

-include $(deps)

.PHONY: all install clean
