PREFIX  ?= /usr/local
DESTDIR ?=
CFLAGS  ?= -ggdb -Wall -Wextra -MMD -MP

BIN_DIR := bin

include ../rules.mk

INCLUDE := -I$(DESTDIR)$(PREFIX)/include
LIBS    := -L$(DESTDIR)$(PREFIX)/lib \
           -Wl,--start-group -lcore -lgeometry -lnet -los -lgraphics -lwm -l$(call wm_backend) -Wl,--end-group \
           $(call wm_libs)

libfiles := $(wildcard $(DESTDIR)$(PREFIX)/lib/*.a)

projects := $(filter-out $(call demo_skip),$(patsubst ./%/main.c,%,$(call rwildcard,.,*main.c)))
binaries := $(foreach p,$(projects),$(BIN_DIR)/$(call executable,$p))
deps     := $(addsuffix .d,$(binaries))

all: $(binaries)

$(BIN_DIR)/$(call executable,%): %/main.c $(libfiles)
	@$(call mkdir_p,$(BIN_DIR))
	$(CC) $(CFLAGS) $(INCLUDE) -MF $@.d -o $@ $(wildcard $*/*.c) $(LIBS)

clean:
	-$(call rm_rf,$(BIN_DIR))

-include $(deps)

.PHONY: all clean
