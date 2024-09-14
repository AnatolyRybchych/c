CC				:= gcc
INCLUDE			:= -Iinclude
CARGS			:= $(INCLUDE) -ggdb -Wall -Wextra -Werror -pedantic
LIBMC			:= mc
LIB_DIR			:= lib
LIBMC_STATIC	:= $(LIB_DIR)/lib$(LIBMC).a
LIBMC_OBJ_DIR	:= build/libmc/obj
LIBMC_SRC_DIR	:= src

objects			+= dlib.o
objects			+= error.o
objects			+= sched.o
objects			+= time.o
objects			+= data/string.o
objects			+= data/vector.o
objects			+= data/pqueue.o
objects			+= data/list.o
objects			+= data/struct.o
objects			+= data/arena.o
objects			+= data/img/bmp.o
objects			+= data/bit.o
objects			+= data/str.o
objects			+= geometry/lina.o
objects			+= geometry/bezier.o
objects			+= io/stream.o

objects			+= os/fd.o
objects			+= os/file.o
objects			+= os/socket.o

objects			+= os/process.o

objects			+= wm/wm.o
objects			+= wm/event.o
objects			+= wm/target.o
objects			+= wm/mouse_button.o
objects			+= wm/key.o

objects			+= graphics/graphics.o
objects			+= graphics/di.o

objects			+= xlib_wm/wm.o
objects			+= xlib_wm/graphics.o

DEMO_BIN		:= bin/demo
DEMO_OBJ_DIR	:= build/demo/obj
DEMO_SRC_DIR	:= demo
DEMO_CARGS		:= -ggdb -Wall -Wextra -Werror -pedantic -Iinclude
DEMO_LDARGS		:= -L$(LIB_DIR) -l$(LIBMC) -lX11 -lm

demo_objects	+= main.o


TEST_BIN		:= bin/test
TEST_OBJ_DIR	:= build/tests/obj
TEST_SRC_DIR	:= tests
TEST_CARGS		:= -ggdb -Wall -Wextra -Werror -pedantic -Iinclude
TEST_LDARGS		:= -static -L$(LIB_DIR) -l$(LIBMC)

test_objects	+= main.o
test_objects	+= time.o


$(LIBMC_STATIC): $(addprefix $(LIBMC_OBJ_DIR)/, $(objects))
	@mkdir -p $(dir $@)
	ar rcs ./$@ $^

$(LIBMC_OBJ_DIR)/%.o: $(LIBMC_SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CARGS) -o $@ $^

libmc: $(LIBMC_STATIC)

# ::: DEMO :::

$(DEMO_BIN): $(addprefix $(DEMO_OBJ_DIR)/, $(demo_objects))
	@mkdir -p $(dir $(DEMO_BIN))
	$(CC) $(DEMO_CARGS) -o $@ $^ $(DEMO_LDARGS)

$(DEMO_OBJ_DIR)/%.o: $(DEMO_SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) -c $(DEMO_CARGS) -o $@ $^

demo: libmc $(DEMO_BIN)

run: demo
	$(DEMO_BIN)

gdb: demo
	if which gf2 >/dev/null; then gf2 $(DEMO_BIN); else gdb $(DEMO_BIN); fi


# ::: TEST :::

$(TEST_BIN): $(addprefix $(TEST_OBJ_DIR)/, $(test_objects))
	@mkdir -p $(dir $(TEST_BIN))
	$(CC) $(TEST_CARGS) -o $@ $^ $(TEST_LDARGS)

$(TEST_OBJ_DIR)/%.o: $(TEST_SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) -c $(TEST_CARGS) -o $@ $^

test: libmc $(TEST_BIN)
	$(TEST_BIN)

all: libmc demo test

clean:
	rm -rf build bin lib

.PHONY: test demo libmc run