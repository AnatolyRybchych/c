CC      := gcc
INCLUDE := -Iinclude
CARGS   := $(INCLUDE) -ggdb -Wall -Wextra -Werror -pedantic
LIB     := mc
OUT     := lib/lib$(LIB).a

objects += mc.o
objects += dlib.o
objects += error.o
objects += sched.o
objects += time.o
objects += data/string.o
objects += data/vector.o

build: $(addprefix obj/, $(objects))
	@mkdir -p $(dir ./$(OUT))
	ar rcs ./$(OUT) $^

obj/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CARGS) -o $@ $^

run: build
	gcc -ggdb $(CARGS) -static -o run run.c -L$(dir $(OUT)) -l$(LIB)
	./run

gdb: build
	gcc -ggdb $(CARGS) -static -o run run.c -L$(dir $(OUT)) -l$(LIB)
	gdb ./run

clean:
	rm $(OUT)
	rm -r ./obj
