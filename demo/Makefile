PREFIX  ?= /usr/local
DESTDIR ?=
CC      ?= gcc
CFLAGS  ?= -ggdb -Wall -Wextra -Werror -pedantic -MMD -MP

# headers and libraries come from the (staging or real) install prefix
INCLUDE := -I$(DESTDIR)$(PREFIX)/include
LIBS    := -L$(DESTDIR)$(PREFIX)/lib \
           -Wl,--start-group -lcore -lgeometry -lnet -los -lgraphics -lwm -lxlib_wm -Wl,--end-group \
           -lX11 -lm

BIN_DIR := bin

# a demo project is any subdirectory holding a main.c; its name is the binary name
projects := $(patsubst ./%/main.c,%,$(shell find . -mindepth 2 -maxdepth 2 -name main.c))
binaries := $(addprefix $(BIN_DIR)/,$(projects))
deps     := $(addsuffix .d,$(binaries))

# relink whenever a staged library changes
libfiles := $(wildcard $(DESTDIR)$(PREFIX)/lib/*.a)

all: $(binaries)

$(BIN_DIR)/%: %/main.c $(libfiles)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDE) -MF $@.d -o $@ $(wildcard $*/*.c) $(LIBS)

clean:
	rm -rf $(BIN_DIR)

-include $(deps)

.PHONY: all clean
