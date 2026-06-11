NAME := lua

PREFIX  ?= /usr/local
DESTDIR ?=
CC      ?= gcc
AR      ?= ar

LUA_REPO   := https://github.com/lua/lua.git
LUA_COMMIT := 6e22fedb74cf0c9b6656e9fce8b7331db847c605
LUA_DIR    := lua
LUA_STAMP  := $(LUA_DIR)/.commit.$(LUA_COMMIT)
LIB        := lib$(NAME).a

include ../../rules.mk

all: $(LIB)

$(LIB): $(LUA_STAMP)
	$(MAKE) -C $(LUA_DIR) a CC=$(CC) MYCFLAGS=
	$(call cp_file,$(LUA_DIR)/liblua.a,$(LIB))

$(LUA_STAMP):
	git init -q $(LUA_DIR)
	git -C $(LUA_DIR) config core.autocrlf false
	git -C $(LUA_DIR) fetch -q --depth 1 $(LUA_REPO) $(LUA_COMMIT)
	git -C $(LUA_DIR) checkout -q --detach FETCH_HEAD
	$(call touch_file,$(LUA_STAMP))

install: $(LIB)
	$(call mkdir_p,$(DESTDIR)$(PREFIX)/lib)
	$(call mkdir_p,$(DESTDIR)$(PREFIX)/include)
	$(call cp_file,$(LIB),$(DESTDIR)$(PREFIX)/lib)
	$(call cp_file,$(LUA_DIR)/lua.h,$(DESTDIR)$(PREFIX)/include)
	$(call cp_file,$(LUA_DIR)/luaconf.h,$(DESTDIR)$(PREFIX)/include)
	$(call cp_file,$(LUA_DIR)/lauxlib.h,$(DESTDIR)$(PREFIX)/include)
	$(call cp_file,$(LUA_DIR)/lualib.h,$(DESTDIR)$(PREFIX)/include)

clean:
	-$(call rm_rf,$(LIB))

.PHONY: all install clean
