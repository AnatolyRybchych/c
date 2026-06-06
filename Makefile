PKG_DIR := package
PKGS ?= core geometry net os graphics wm xlib_wm

PREFIX  ?= /usr/local
DESTDIR ?=

STAGE   := $(CURDIR)/build

all: libs demos mutation

libs:
	$(foreach p,$(PKGS),$(MAKE) -C $(PKG_DIR)/$(p) install PREFIX= DESTDIR=$(STAGE) &&) true

demos: libs
	$(MAKE) -C demo PREFIX= DESTDIR=$(STAGE)

mutation: libs
	$(MAKE) -C mutation PREFIX= DESTDIR=$(STAGE)

install:
	$(foreach p,$(PKGS),$(MAKE) -C $(PKG_DIR)/$(p) install PREFIX=$(PREFIX) DESTDIR=$(DESTDIR) &&) true

clean:
	$(foreach p,$(PKGS),$(MAKE) -C $(PKG_DIR)/$(p) clean &&) true
	$(MAKE) -C demo clean
	$(MAKE) -C mutation clean
	rm -rf build

.PHONY: all libs demos mutation install clean
