PKG_DIR := package
# topological order: each package's deps are built+staged before it
PKGS    := core geometry net os graphics wm xlib_wm

PREFIX  ?= /usr/local
DESTDIR ?=

# staging prefix: packages install their headers/libs here so later packages,
# the demos and the tests compile/link against build/include and build/lib
STAGE   := $(CURDIR)/build

all: libs demos mutation

# build each package and stage it into $(STAGE); deps are already staged
libs:
	$(foreach p,$(PKGS),$(MAKE) -C $(PKG_DIR)/$(p) install PREFIX= DESTDIR=$(STAGE) &&) true

demos: libs
	$(MAKE) -C demo PREFIX= DESTDIR=$(STAGE)

mutation: libs
	$(MAKE) -C mutation PREFIX= DESTDIR=$(STAGE)

# install every package into $(DESTDIR)$(PREFIX), deps first
install:
	$(foreach p,$(PKGS),$(MAKE) -C $(PKG_DIR)/$(p) install PREFIX=$(PREFIX) DESTDIR=$(DESTDIR) &&) true

clean:
	$(foreach p,$(PKGS),$(MAKE) -C $(PKG_DIR)/$(p) clean &&) true
	$(MAKE) -C demo clean
	$(MAKE) -C mutation clean
	rm -rf build

.PHONY: all libs demos mutation install clean
