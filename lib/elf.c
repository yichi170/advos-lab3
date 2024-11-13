#include "myelf.h"

#include <fcntl.h> // open
#include <unistd.h> // read
#include <stdint.h>
#include <stdlib.h> // EXIT_FAILURE
#include <string.h> // memcmp
#include <sys/mman.h> // mmap

#include "lib.h"

extern void err_quit(const char *msg);

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
			void *mapped_mem = mmap(segment_vaddr, segment_size,
						PROT_READ | PROT_WRITE,
						MAP_PRIVATE | MAP_ANONYMOUS, -1,
						0);

			printf("segment_vaddr (ideal): %#lx\n",
			       (uint64_t)segment_vaddr);
			printf("mapped vaddr (actual): %#lx\n",
			       (uint64_t)mapped_mem);

			if (mapped_mem == MAP_FAILED) {
				free(phdrs);
				close(fd);
				err_quit(
					"mmap failed to allocate memory for segment");
			}

			lseek(fd, phdr->p_offset, SEEK_SET);
			if (read(fd, mapped_mem, segment_file_size) !=
			    segment_file_size) {
				free(phdrs);
				close(fd);
				err_quit("Load segment to memory");
			}

			// zero-out the remaining segment space (.bss section)
			if (segment_size > segment_file_size) {
				memset((char *)mapped_mem + segment_file_size,
				       0, segment_size - segment_file_size);
			}

			munmap(mapped_mem, segment_size);
		}
	}

	free(phdrs);
	close(fd);
}