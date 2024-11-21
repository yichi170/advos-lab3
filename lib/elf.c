#include "myelf.h"

#include <fcntl.h> // open
#include <unistd.h> // read
#include <stdint.h>
#include <stdlib.h> // EXIT_FAILURE
#include <string.h> // memcmp
#include <sys/mman.h> // mmap

#include "lib.h"
#include "stack_util.h"

extern void err_quit(const char *msg);

elf_t *parse_elf_headers(const char *elf_file)
{
	int fd = open(elf_file, O_RDONLY);
	if (fd == -1) {
		err_quit("Open ELF format file");
	}

	elf_t *elf = malloc(sizeof(elf_t));

	elf->filename = malloc(strlen(elf_file) + 1);
	strcpy(elf->filename, elf_file);

	Elf64_Ehdr *ehdr = &elf->ehdr;
	if (read(fd, ehdr, sizeof(Elf64_Ehdr)) != sizeof(Elf64_Ehdr)) {
		close(fd);
		err_quit("Read ELF header");
	}

	if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0) {
		err_quit("This file is not in ELF format");
	}

	elf->phdrs = malloc(ehdr->e_phentsize * ehdr->e_phnum);
	Elf64_Phdr *phdrs = elf->phdrs;

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

	return elf;
}

void setup_stack_exec(elf_t* elf, void* entry_point, char** argv, char** envp) {
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

	Elf64_auxv_t *auxp = (Elf64_auxv_t *)(envp + envc + 1);
	int auxc = 0;
	while (1) {
		stack_ptr -= sizeof(Elf64_auxv_t);
		auxc++;

		// ensure AT_NULL will be copied to the stack
		if (auxp->a_type == AT_NULL) {
			break;
		}
		auxp++;
	}
	auxp = (Elf64_auxv_t *)(envp + envc + 1);
	memcpy(stack_ptr, auxp, sizeof(Elf64_auxv_t) * auxc);

	Elf64_auxv_t *stk_auxp = (Elf64_auxv_t *)stack_ptr;
	while (stk_auxp->a_type != AT_NULL) {
		// printf("stk_auxp: %#lx %#lx\n", stk_auxp->a_type, stk_auxp->a_un.a_val);
		if (stk_auxp->a_type == AT_PHDR) {
			stk_auxp->a_un.a_val = (uint64_t)elf->phdrs;
		} else if (stk_auxp->a_type == AT_PHNUM) {
			stk_auxp->a_un.a_val = elf->ehdr.e_phnum;
		} else if (stk_auxp->a_type == AT_BASE) {
			stk_auxp->a_un.a_val = 0;
		} else if (stk_auxp->a_type == AT_ENTRY) {
			stk_auxp->a_un.a_val = elf->ehdr.e_entry;
		} else if (stk_auxp->a_type == AT_EXECFN) {
			stk_auxp->a_un.a_val = (uint64_t)elf->filename;
		}
		// printf("stk_auxp: %#lx %#lx\n", stk_auxp->a_type, stk_auxp->a_un.a_val);
		stk_auxp++;
	}

	stack_ptr -= sizeof(char *) * (envc + 1);
	char **envp_stack = (char **)stack_ptr;
	memcpy(envp_stack, envp, sizeof(char *) * (envc + 1));

	stack_ptr -= sizeof(char *) * (argc + 1);
	char **argv_stack = (char **)stack_ptr;
	memcpy(argv_stack, argv, sizeof(char *) * (argc + 1));

	stack_ptr -= sizeof(size_t);
	*stack_ptr = argc;

	stack_ptr = (char *)((uintptr_t)stack_ptr & ~0xF);

	stack_check(stack_ptr, argc, argv);

	printf("jump to entry point: %#lx\n", (uintptr_t)entry_point);

	__asm__ volatile (
		"mov x0, #0\n"
		"mov x1, #0\n"
		"mov x2, #0\n"
		"mov x3, #0\n"
		"mov x4, #0\n"
		"mov x5, #0\n"
		"mov x6, #0\n"
		"mov x7, #0\n"
		"mov sp, %0\n"
		"br %1\n"
		: : "r" (stack_ptr), "r" (entry_point)
	);

	err_quit("Should not reach here");
}
