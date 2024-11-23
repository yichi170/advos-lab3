#include "myelf.h"

#include <fcntl.h> // open
#include <unistd.h> // read
#include <stdint.h>
#include <stdlib.h> // EXIT_FAILURE
#include <string.h> // memcmp
#include <sys/mman.h> // mmap

#include "lib.h"

static size_t count_arg(char **args)
{
	int argc = 0;
	for (char **arg = args; *arg != NULL; ++arg) {
		argc++;
	}
	return argc;
}

elf_t *parse_elf_headers(const char *elf_file)
{
	elf_t *elf = malloc(sizeof(elf_t));
	elf->fd = open(elf_file, O_RDONLY);
	if (elf->fd == -1) {
		err_quit("Open ELF format file");
	}

	elf->filename = malloc(strlen(elf_file) + 1);
	strcpy(elf->filename, elf_file);

	Elf64_Ehdr *ehdr = &elf->ehdr;
	if (read(elf->fd, ehdr, sizeof(Elf64_Ehdr)) != sizeof(Elf64_Ehdr)) {
		close(elf->fd);
		err_quit("Read ELF header");
	}

	if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0) {
		err_quit("This file is not in ELF format");
	}

	elf->phdrs = malloc(ehdr->e_phentsize * ehdr->e_phnum);
	Elf64_Phdr *phdrs = elf->phdrs;

	if (!phdrs) {
		close(elf->fd);
		err_quit("Memory allocation for program headers");
	}

	lseek(elf->fd, ehdr->e_phoff, SEEK_SET);
	if (read(elf->fd, phdrs, ehdr->e_phentsize * ehdr->e_phnum) !=
		ehdr->e_phentsize * ehdr->e_phnum) {
		free(phdrs);
		close(elf->fd);
		err_quit("Reading program headers");
	}

	return elf;
}

char *setup_stack(elf_t* elf, char** argv, char** envp) {
	size_t argc = count_arg(argv);
	size_t envc = count_arg(envp);
	const int STACK_SIZE = 0x800000; // 8MB
	char *stack_top;
	char *stack_ptr;

	stack_top = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE,
						   MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
	if (stack_top == MAP_FAILED) {
		err_quit("Failed to allocate memory for the stack");
	}

	stack_ptr = stack_top + STACK_SIZE;
	stack_ptr = (char *)((uintptr_t)stack_ptr & ~0xF);

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

	return stack_ptr;
}
