NAME := os

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

include ../../rules.mk

sources := $(filter-out $(addprefix $(SRC_DIR)/,$(EXCLUDE)),$(call rwildcard,$(SRC_DIR),*.c))
objects := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(sources))
deps    := $(objects:.o=.d)

all: $(LIB)

$(LIB): $(objects)
	$(AR) rcs $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@$(call mkdir_p,$(@D))
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<

install: $(LIB)
	mkdir -p $(DESTDIR)$(PREFIX)/lib $(DESTDIR)$(PREFIX)/include/mc
	cp -p $(LIB) $(DESTDIR)$(PREFIX)/lib/
	cp -rp include/mc/. $(DESTDIR)$(PREFIX)/include/mc/

clean:
	$(RM) $(OBJ_DIR) $(LIB)

-include $(deps)

.PHONY: all install clean
