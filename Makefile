PKG_DIR := package
# topological order: each package's deps are built+staged before it
PKGS    := core geometry net os graphics wm xlib_wm

CC      ?= gcc
PREFIX  ?= /usr/local
DESTDIR ?=

# staging prefix: packages install their headers/libs here so later packages,
# the demo and the tests compile/link against build/include and build/lib
STAGE   := $(CURDIR)/build

INCLUDE := -I$(STAGE)/include
CARGS   := $(INCLUDE) -ggdb -Wall -Wextra -Werror -pedantic

# libraries the demo and tests link against (most-dependent first)
LINK     := os net core
LDARGS   := -L$(STAGE)/lib $(addprefix -l,$(LINK))

MUTATION_TEST_SRC := $(shell find mutation -type f -name main.c | sed s!/main.c!!g)

all: libs demos mutation

# build each package and stage it into $(STAGE); deps are already staged
libs:
	$(foreach p,$(PKGS),$(MAKE) -C $(PKG_DIR)/$(p) install PREFIX= DESTDIR=$(STAGE) &&) true

demos: libs
	$(MAKE) -C demo PREFIX= DESTDIR=$(STAGE)

mutation: libs
	$(foreach test,$(MUTATION_TEST_SRC), \
		$(CC) $(CARGS) -o $(test)/run $(test)/main.c $(LDARGS) && \
		$(test)/run > $(test)/stdout 2> $(test)/stderr; \
		rm -f $(test)/run;)

# install every package into $(DESTDIR)$(PREFIX), deps first
install:
	$(foreach p,$(PKGS),$(MAKE) -C $(PKG_DIR)/$(p) install PREFIX=$(PREFIX) DESTDIR=$(DESTDIR) &&) true

clean:
	$(foreach p,$(PKGS),$(MAKE) -C $(PKG_DIR)/$(p) clean &&) true
	$(MAKE) -C demo clean
	rm -rf build

.PHONY: all libs demos mutation install clean
