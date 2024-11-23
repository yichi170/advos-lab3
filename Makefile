CC = gcc
CFLAGS = -g -Wall -O2 -Iinclude

LIB_SRC := $(wildcard lib/*.c)
LIB_OBJ := $(LIB_SRC:%.c=%.o)
# LIB_DEP := $(LIB_SRC:%.c=%.d)

SRC := $(wildcard *.c)
OBJ := $(SRC:.c=.o)
EXEC := $(SRC:.c=)


all: $(EXEC)

%: %.c $(LIB_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ -static

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

lib_objs: $(LIB_OBJ)

clean:
	$(RM) $(EXEC) $(LIB_OBJ)
	make clean -C test/

test:
	make -C test/

.PHONY: all test clean lib_objs
