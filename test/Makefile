SRC := $(wildcard *.c)
OBJ := $(SRC:.c=.o)
EXEC := $(SRC:.c=)

all: $(EXEC)

%: %.c linker.ld
	gcc -g -o $@ $< -static -T linker.ld

clean:
	$(RM) $(EXEC) $(OBJ)

.PHONY: all clean

