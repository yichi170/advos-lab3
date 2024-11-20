#include "myelf.h"

#include <fcntl.h> // open
#include <unistd.h> // read
#include <stdint.h>
#include <stdlib.h> // EXIT_FAILURE
#include <string.h> // memcmp
#include <sys/mman.h> // mmap

#include "lib.h"

extern void err_quit(const char *msg);

Elf64_Phdr *parse_elf_headers(const char *elf_file, Elf64_Ehdr *ehdr)
{
	int fd = open(elf_file, O_RDONLY);
	if (fd == -1) {
		err_quit("Open ELF format file");
	}

	if (read(fd, ehdr, sizeof(*ehdr)) != sizeof(*ehdr)) {
		close(fd);
		err_quit("Read ELF header");
	}

	if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0) {
		err_quit("This file is not in ELF format");
	}

	Elf64_Phdr *phdrs = malloc(ehdr->e_phentsize * ehdr->e_phnum);

	if (!phdrs) {
		close(fd);
		err_quit("Memory allocation for program headers");
	}

	lseek(fd, ehdr->e_phoff, SEEK_SET);
	if (read(fd, phdrs, ehdr->e_phentsize * ehdr->e_phnum) !=
		ehdr->e_phentsize * ehdr->e_phnum) {
		free(phdrs);
		close(fd);
		err_quit("Reading program headers");
	}

	close(fd);

	return phdrs;
}

void setup_stack_exec(void* entry_point, char** argv, char** envp) {
	size_t argv_size = 0, envp_size = 0;
	size_t argc = 0, envc = 0;
	const int STACK_SIZE = 0x800000; // 8MB
	char *stack_top;
	char *stack_ptr;

	for (char **arg = argv; *arg != NULL; ++arg) {
		argv_size += strlen(*arg) + 1;
		argc++;
	}

	for (char **env = envp; *env != NULL; ++env) {
		envp_size += strlen(*env) + 1;
		envc++;
	}

	size_t args_size = argv_size + envp_size + sizeof(char *) * (envc + argc);
	size_t stack_size = (args_size > STACK_SIZE) ? args_size : STACK_SIZE;

	stack_top = mmap(NULL, stack_size, PROT_READ | PROT_WRITE,
						   MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
	if (stack_top == MAP_FAILED) {
		err_quit("Failed to allocate memory for the stack");
	} else {
		printf("Stack allocated at: %p\n", stack_top);
	}

	stack_ptr = stack_top + stack_size;

	stack_ptr -= sizeof(size_t);
	*stack_ptr = argc;

	stack_ptr -= sizeof(char *) * (argc + 1);
	char **argv_stack = (char **)stack_ptr;
	char **argv_ptr = argv_stack;

	stack_ptr -= sizeof(char *) * (envc + 1);
	char **envp_stack = (char **)stack_ptr;
	char **envp_ptr = envp_stack;

	for (char **arg = argv; *arg != NULL; ++arg) {
		stack_ptr -= (strlen(*arg) + 1);
		strcpy(stack_ptr, *arg);
		*argv_ptr = stack_ptr;
		++argv_ptr;
	}
	*argv_ptr = NULL;

	for (char **env = envp; *env != NULL; ++env) {
		stack_ptr -= (strlen(*env) + 1); 	// strlen doesn't count '\0'
		strcpy(stack_ptr, *env); 			// strcpy would copy '\0'
		*envp_ptr = stack_ptr;
		envp_ptr++;
	}
	*envp_ptr = NULL;

	stack_ptr = (char *)((uintptr_t)stack_ptr & ~0xF);

	printf("jump to entry point: %#lx\n", (uintptr_t)entry_point);

	__asm__ volatile (
		"mov x0, %2\n"
		"mov x1, %3\n"
		"mov x2, %4\n"
		"mov sp, %0\n"
		"br %1\n"
		: : "r" (stack_ptr), "r" (entry_point), "r"(argc), "r"(argv_stack), "r"(envp_stack)
	);

	err_quit("Should not reach here");
}
