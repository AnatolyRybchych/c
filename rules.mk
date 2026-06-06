ifeq ($(OS),Windows_NT)
    TARGET ?= windows
else
    TARGET ?= linux
endif

RM := rm -rf

ifeq ($(TARGET),windows)
    mkdir_p = if not exist "$(subst /,\,$1)" mkdir "$(subst /,\,$1)"
else
    mkdir_p = mkdir -p $1
endif

rwildcard = $(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))
