#ifndef LIB_ELF_H
#define LIB_ELF_H

#include <elf.h>

Elf64_Phdr *parse_elf_headers(const char *buf, Elf64_Ehdr *ehdr);
void setup_stack_exec(void *entry_point, char **argv, char **envp);

#endif
