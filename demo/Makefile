PREFIX  ?= /usr/local
DESTDIR ?=
CC      ?= gcc
CFLAGS  ?= -ggdb -Wall -Wextra -Werror -pedantic

# headers and libraries come from the (staging or real) install prefix
INCLUDE := -I$(DESTDIR)$(PREFIX)/include
LIBS    := -L$(DESTDIR)$(PREFIX)/lib \
           -Wl,--start-group -lcore -lgeometry -lnet -los -lgraphics -lwm -lxlib_wm -Wl,--end-group \
           -lX11 -lm

BIN_DIR := bin

# a demo project is any subdirectory holding a main.c; its name is the binary name
projects := $(patsubst ./%/main.c,%,$(shell find . -mindepth 2 -maxdepth 2 -name main.c))
binaries := $(addprefix $(BIN_DIR)/,$(projects))

all: $(binaries)

$(BIN_DIR)/%: %/main.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $(wildcard $*/*.c) $(LIBS)

clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean
