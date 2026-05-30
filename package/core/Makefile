NAME := core

PREFIX  ?= /usr/local
DESTDIR ?=
CC      ?= gcc
AR      ?= ar
CFLAGS  ?= -ggdb -Wall -Wextra -Werror -pedantic -MMD -MP

# dependency headers come from the (staging or real) install prefix
INCLUDE := -Iinclude -I$(DESTDIR)$(PREFIX)/include

SRC_DIR := src
OBJ_DIR := build/obj
LIB     := lib$(NAME).a

sources := $(shell find $(SRC_DIR) -name '*.c')
objects := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(sources))
deps    := $(objects:.o=.d)

all: $(LIB)

$(LIB): $(objects)
	$(AR) rcs $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<

install: $(LIB)
	mkdir -p $(DESTDIR)$(PREFIX)/lib $(DESTDIR)$(PREFIX)/include/mc
	cp -p $(LIB) $(DESTDIR)$(PREFIX)/lib/
	cp -rp include/mc/. $(DESTDIR)$(PREFIX)/include/mc/

clean:
	rm -rf $(OBJ_DIR) $(LIB)

-include $(deps)

.PHONY: all install clean
