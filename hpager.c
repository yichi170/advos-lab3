#include <fcntl.h>
#include <unistd.h> // read
#include <stdio.h>
#include <stdlib.h> // exit
#include <sys/mman.h> // mmap
#include <string.h>
#include <assert.h>
#include <signal.h>

#include "myelf.h"
#include "lib.h"
#include "jmp_exec.h"
#include "stack_util.h"

#define PAGE_SIZE 0x1000

elf_t *elf;

__attribute__ ((unused))
static uint64_t min(uint64_t a, uint64_t b) {
	return (a > b) ? b : a;
}

__attribute__ ((unused))
static uint64_t max(uint64_t a, uint64_t b) {
	return (a > b) ? a : b;
}

// int cnt = 0;
void signal_handler(int signal, siginfo_t *si, void *unused) {
	if (signal != SIGSEGV) {
		err_quit("Received unknown signal");
	}
	// printf("si_addr: %#lx,\t", (uint64_t)si->si_addr);

	for (int i = 0; i < elf->ehdr.e_phnum; i++) {
		Elf64_Phdr *phdr = &elf->phdrs[i];
		if (phdr->p_type != PT_LOAD)
			continue;

		if (phdr->p_vaddr <= (uint64_t)si->si_addr &&
			(uint64_t)si->si_addr < phdr->p_vaddr + phdr->p_memsz) {
			// cnt++;
			uint64_t start_addr = (uint64_t)si->si_addr & ~0xFFF;
			uint64_t end_addr = start_addr + PAGE_SIZE;
			// printf("start_addr: %#lx\n", start_addr);
			void *mapped_mem = mmap((void *)start_addr, PAGE_SIZE,
				PROT_READ | PROT_WRITE | PROT_EXEC,
				MAP_PRIVATE | MAP_ANONYMOUS, -1,
				0);

			if (mapped_mem == MAP_FAILED) {
				err_quit("mmap failed to allocate memory for segment");
			// } else {
			// 	printf("mmap addr: %#lx\n", (uint64_t)mapped_mem);
			}

			void *mapped_mem2 = mmap((void *)start_addr + PAGE_SIZE, PAGE_SIZE,
				PROT_READ | PROT_WRITE | PROT_EXEC,
				MAP_PRIVATE | MAP_ANONYMOUS, -1,
				0);

			if (mapped_mem2 == MAP_FAILED) {
				err_quit("mmap failed to allocate memory for segment");
			}
			// This memory region is already allocated!
			if ((uint64_t)mapped_mem2 != start_addr + PAGE_SIZE) {
				// printf("The next page is allocated\n");
				munmap(mapped_mem2, PAGE_SIZE);
			}

			if (start_addr < phdr->p_vaddr) {
				start_addr = phdr->p_vaddr;
			}

			if (end_addr > phdr->p_vaddr + phdr->p_memsz) {
				end_addr = phdr->p_vaddr + phdr->p_memsz;
			}

			if (end_addr > phdr->p_vaddr + phdr->p_filesz) {
				uint64_t memset_start = max(phdr->p_vaddr + phdr->p_filesz, start_addr);
				// printf("memset: %#lx - %#lx\n", memset_start, end_addr);
				memset((char *)memset_start, 0, end_addr - memset_start);
			}
		}
	}
	// printf("cnt: %d\n", cnt);
}

int main(int argc, char **argv, char **envp)
{
	if (argc < 2) {
		fprintf(stderr, "USAGE: %s <program to load>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	char *program = argv[1];
	printf("load program %s\n", program);

	elf = parse_elf_headers(program);

	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = signal_handler;
	sa.sa_flags = SA_SIGINFO;

	if (sigaction(SIGSEGV, &sa, NULL) == -1) {
			err_quit("Error setting up sigaction");
	}

	// load to memory
	for (int i = 0; i < elf->ehdr.e_phnum; i++) {
		Elf64_Phdr *phdr = &elf->phdrs[i];
		if (phdr->p_type != PT_LOAD) {
			continue;
		}

		// only allcoate text section with size = PAGE_SIZE
		uint64_t shifted_vaddr = phdr->p_vaddr & ~0xFFF;
		uint64_t padding_size = phdr->p_vaddr - shifted_vaddr;
		uint64_t segment_fsize = phdr->p_filesz;

		void *mapped_mem = mmap((void *)shifted_vaddr, segment_fsize + padding_size,
					PROT_READ | PROT_WRITE | PROT_EXEC,
					MAP_PRIVATE | MAP_ANONYMOUS, -1,
					0);

		if (mapped_mem == MAP_FAILED) {
			err_quit(
				"mmap failed to allocate memory for segment");
		}
		assert((uint64_t)mapped_mem == shifted_vaddr);

		void *mapped_ptr = (void *)((uint64_t)mapped_mem + padding_size);

		lseek(elf->fd, phdr->p_offset, SEEK_SET);
		if (read(elf->fd, mapped_ptr, segment_fsize) != segment_fsize) {
			err_quit("Load segment to memory");
		}

		printf("==============================\n");
		printf("padding size: %#lx\n", padding_size);
		printf("segment_flags [r/w/x]: %c%c%c\n",
			" r"[(phdr->p_flags & PF_R) != 0],
			" w"[(phdr->p_flags & PF_W) != 0],
			" x"[(phdr->p_flags & PF_X) != 0]);
		printf("shifted segment vaddr: %#lx\n", (uint64_t)shifted_vaddr);
		printf("writing data from this addr: %#lx\n", (uint64_t)mapped_ptr);
		printf("segment mapped to this region: %#lx - %#lx\n",
			(uint64_t)mapped_mem,
			(uint64_t)mapped_mem + segment_fsize);
		printf("==============================\n");
	}

	char *stack = setup_stack(elf, argv + 1, envp);
	stack_check(stack, argc - 1, argv + 1);

	jump_exec(stack, (void *)elf->ehdr.e_entry);

	return 0;
}
