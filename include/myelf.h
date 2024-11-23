#ifndef LIB_ELF_H
#define LIB_ELF_H

#include <elf.h>

typedef struct elf_s {
    char* filename;
    Elf64_Ehdr  ehdr;
    Elf64_Phdr* phdrs;
    int fd;
} elf_t;

elf_t *parse_elf_headers(const char* buf);
char *setup_stack(elf_t* elf, char** argv, char** envp);

#endif
