#ifndef LIB_ELF_H
#define LIB_ELF_H

#include <elf.h>

typedef struct elf_s {
    char *filename;
    Elf64_Ehdr  ehdr;
    Elf64_Phdr  *phdrs;
} elf_t;

elf_t *parse_elf_headers(const char *buf);
void setup_stack_exec(elf_t* elf, void *entry_point, char **argv, char **envp);

#endif
