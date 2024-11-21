#include <fcntl.h>
#include <unistd.h> // read
#include <stdio.h>
#include <stdlib.h> // exit
#include <sys/mman.h> // mmap
#include <string.h>

#include "myelf.h"
#include "lib.h"

void test_entry(int argc, char **argv, char **envp)
{
	printf("\n\nProgram started with argc: %d\n", argc);

	for (int i = 0; i < argc; i++) {
		printf("argv[%d] = %s\n", i, argv[i]);
	}

	for (char **env = envp; *env != NULL; ++env) {
		printf("%s\n", *env);
	}
	exit(1);
}

int main(int argc, char **argv, char **envp)
{
	if (argc != 2) {
		fprintf(stderr, "USAGE: %s <program to load>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	char *program = argv[1];
	printf("load program %s\n", program);

	elf_t *elf = parse_elf_headers(program);
	uint64_t load_bias = 0;

	int fd = open(program, O_RDONLY);
	if (fd == -1) {
		err_quit("Open ELF format file");
	}

	// load to memory
	for (int i = 0; i < elf->ehdr.e_phnum; i++) {
		Elf64_Phdr *phdr = &elf->phdrs[i];
		if (phdr->p_type == PT_LOAD) {
			void *segment_vaddr = (void *)phdr->p_vaddr;
			size_t segment_size = phdr->p_memsz;
			size_t segment_file_size = phdr->p_filesz;
			void *mapped_mem = mmap(segment_vaddr, segment_size,
						PROT_READ | PROT_WRITE | PROT_EXEC,
						MAP_PRIVATE | MAP_ANONYMOUS, -1,
						0);

			if (mapped_mem == MAP_FAILED) {
				free(elf);
				err_quit(
					"mmap failed to allocate memory for segment");
			}

			load_bias = (uint64_t)mapped_mem - (uint64_t)segment_vaddr;

			lseek(fd, phdr->p_offset, SEEK_SET);
			if (read(fd, mapped_mem, segment_file_size) !=
				segment_file_size) {
				free(elf);
				err_quit("Load segment to memory");
			}

			// zero-out the remaining segment space (.bss section)
			if (segment_size > segment_file_size) {
				memset((char *)mapped_mem + segment_file_size,
					   0, segment_size - segment_file_size);
			}
			printf("load_bias: %#lx\n", load_bias);
			printf("mapped_mem: %#lx\n", (uint64_t)mapped_mem);
			printf("segment_vaddr: %#lx\n", (uint64_t)segment_vaddr);
			printf("segment_size: %#lx\n", segment_size);

			if (elf->ehdr.e_entry < (uint64_t)mapped_mem || elf->ehdr.e_entry > (uint64_t)mapped_mem + segment_size) {
				printf("invalid entry point\n");
			}
		}
	}
	free(elf);
	close(fd);

	unsigned long main_addr = seek_main_tag(program, &elf->ehdr);

	printf("entry point: %#lx\n", elf->ehdr.e_entry);
	printf("shifted entry point: %#lx\n", elf->ehdr.e_entry + load_bias);
	// printf("entry point: %#lx\n", (uint64_t)((void *)test_entry));

	setup_stack_exec(elf, (void *)test_entry, argv + 1, envp);
	// setup_stack_exec(elf, (void *)elf->ehdr.e_entry, argv + 1, envp);

	return 0;
}
