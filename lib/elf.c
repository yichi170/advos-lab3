#include "elf.h"
#include <stdlib.h> // EXIT_FAILURE
#include <string.h> // memcmp
#include <sys/mman.h> // mmap

void load_elf_binary(const char *elf_file)
{
	int fd = open(elf_file, O_RDONLY);
	if (fd == -1) {
		err_quit("Open ELF format file");
	}

	Elf64_Ehdr ehdr;
	if (read(fd, &ehdr, sizeof(ehdr)) != sizeof(ehdr)) {
		close(fd);
		err_quit("Read ELF header");
	}

	if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0) {
		err_quit("This file is not in ELF format");
	}

	Elf64_Phdr *phdrs = malloc(ehdr.e_phentsize * ehdr.e_phnum);

	if (!phdrs) {
		close(fd);
		err_quit("Memory allocation for program headers");
	}

	lseek(fd, ehdr.e_phoff, SEEK_SET);
	if (read(fd, phdrs, ehdr.e_phentsize * ehdr.e_phnum) !=
		ehdr.e_phentsize * ehdr.e_phnum) {
		free(phdrs);
		close(fd);
		err_quit("Reading program headers");
	}

	for (int i = 0; i < ehdr.e_phnum; i++) {
		Elf64_Phdr *phdr = &phdrs[i];

		if (phdr->p_type == PT_LOAD) {
			void *segment_vaddr = (void *)phdr->p_vaddr;
			size_t segment_size = phdr->p_memsz;
			size_t segment_file_size = phdr->p_filesz;
			void *mapped_mem = mmap(
				segment_vaddr, segment_size,
				((phdr->p_flags & PF_X ? PROT_EXEC : 0) |
				 (phdr->p_flags & PF_W ? PROT_WRITE : 0) |
				 (phdr->p_flags & PF_R ? PROT_READ : 0)),
				MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		}
	}

	return;
}