#include <stdio.h>
#include <stdlib.h> // exit
#include <sys/mman.h> // mmap

#include "myelf.h"

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "USAGE: %s <program to load>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	char *program = argv[1];
	printf("load program %s\n", program);

	load_elf_binary(program);

	return 0;
}
