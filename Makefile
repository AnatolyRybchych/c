PKG_DIR := package

PREFIX  ?= /usr/local
DESTDIR ?=

STAGE   := $(CURDIR)/build

include rules.mk

PKGS := core geometry net os graphics wm $(call wm_backend) lua wm-lua

all: libs
	$(foreach s,$(call subprojects), \
		$(MAKE) -C $s DESTDIR=$(STAGE) PREFIX= &&) cd .

libs:
	$(foreach p,$(PKGS), \
		$(MAKE) -C $(PKG_DIR)/$(p) install DESTDIR=$(STAGE) PREFIX= &&) cd .

demos: libs
	$(MAKE) -C demo DESTDIR=$(STAGE) PREFIX=

mutation: libs
	$(MAKE) -C mutation DESTDIR=$(STAGE) PREFIX=

install:
	$(foreach p,$(PKGS), \
		$(MAKE) -C $(PKG_DIR)/$(p) install DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) &&) cd .

clean:
	$(foreach p,$(PKGS), \
		$(MAKE) -C $(PKG_DIR)/$(p) clean &&) cd .
	$(foreach s,$(call subprojects), \
		$(MAKE) -C $s clean &&) cd .
	-$(call rm_rf,build)

.PHONY: all libs demos mutation install clean
