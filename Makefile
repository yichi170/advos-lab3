CC = gcc
CFLAGS = -Wall -O2 -Iinclude

LIB_SRC := $(wildcard lib/*.c)
LIB_OBJ := $(LIB_SRC:%.c=%.o)
# LIB_DEP := $(LIB_SRC:%.c=%.d)

SRC := $(wildcard *.c)
OBJ := $(SRC:.c=.o)
EXEC := $(SRC:.c=)


all: $(EXEC)

%: %.c $(LIB_OBJ)
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

lib_objs: $(LIB_OBJ)

clean:
	$(RM) $(EXEC) $(LIB_OBJ)

.PHONY: all clean lib_objs
