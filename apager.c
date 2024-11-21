#include <fcntl.h>
#include <unistd.h> // read
#include <stdio.h>
#include <stdlib.h> // exit
#include <sys/mman.h> // mmap
#include <string.h>
#include <assert.h>

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
			uint64_t shifted_vaddr = phdr->p_vaddr & ~0xFFF;

			uint64_t padding_size = phdr->p_vaddr - shifted_vaddr;
			printf("==============================\n");
			printf("padding size: %#lx\n", padding_size);

			void *mapped_mem = mmap((void *)shifted_vaddr, segment_size + padding_size,
						PROT_READ | PROT_WRITE | PROT_EXEC,
						MAP_PRIVATE | MAP_ANONYMOUS, -1,
						0);

			if (mapped_mem == MAP_FAILED) {
				free(elf);
				err_quit(
					"mmap failed to allocate memory for segment");
			}

			void *mapped_ptr = (void *)((uint64_t)mapped_mem + padding_size);

			assert((uint64_t)mapped_mem == shifted_vaddr);

			lseek(fd, phdr->p_offset, SEEK_SET);
			if (read(fd, mapped_ptr, segment_file_size) !=
				segment_file_size) {
				free(elf);
				err_quit("Load segment to memory");
			}

			// zero-out the remaining segment space (.bss section)
			if (segment_size > segment_file_size) {
				memset((char *)mapped_ptr + segment_file_size,
					   0, segment_size - segment_file_size);
			}

			char flags[7];
			sprintf(flags, "%c%c%c",
				" r"[(phdr->p_flags & PF_R) != 0],
				" w"[(phdr->p_flags & PF_W) != 0],
				" x"[(phdr->p_flags & PF_X) != 0]);
			printf("segment_flags [r/w/x]: %s\n", flags);
			printf("segment vaddr specified in ELF: %#lx\n", (uint64_t)segment_vaddr);
			printf("shifted segment vaddr: %#lx\n", (uint64_t)shifted_vaddr);
			printf("writing data from this addr: %#lx\n", (uint64_t)mapped_ptr);
			printf("segment mapped to this region: %#lx - %#lx\n", (uint64_t)mapped_mem, (uint64_t)mapped_mem + segment_size);
			printf("==============================\n");

			if (elf->ehdr.e_entry < (uint64_t)mapped_mem || elf->ehdr.e_entry > (uint64_t)mapped_mem + segment_size) {
				printf("invalid entry point\n");
			}
		}
	}
	free(elf);
	close(fd);

	printf("entry point: %#lx\n", elf->ehdr.e_entry);
	printf("shifted entry point: %#lx\n", elf->ehdr.e_entry + load_bias);
	// printf("entry point: %#lx\n", (uint64_t)((void *)test_entry));

	// setup_stack_exec(elf, (void *)test_entry, argv + 1, envp);
	setup_stack_exec(elf, (void *)elf->ehdr.e_entry, argv + 1, envp);

	return 0;
}
