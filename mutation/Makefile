PREFIX  ?= /usr/local
DESTDIR ?=
CC      ?= gcc
CFLAGS  ?= -ggdb -Wall -Wextra -Werror -pedantic

# headers and libraries come from the (staging or real) install prefix
INCLUDE := -I$(DESTDIR)$(PREFIX)/include
LIBS    := -L$(DESTDIR)$(PREFIX)/lib -los -lnet -lcore

# every directory holding a main.c is a golden-snapshot test
TESTS := $(patsubst %/main.c,%,$(shell find . -type f -name main.c))

# build each test, run it, and overwrite its committed stdout/stderr in place;
# regressions show up as a `git diff` (there is no pass/fail exit code)
all:
	$(foreach test,$(TESTS), \
		$(CC) $(CFLAGS) $(INCLUDE) -o $(test)/run $(test)/main.c $(LIBS) && \
		$(test)/run > $(test)/stdout 2> $(test)/stderr; \
		rm -f $(test)/run;)

clean:
	find . -name run -delete

.PHONY: all clean
