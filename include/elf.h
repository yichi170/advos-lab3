#ifndef LIB_ELF_H
#define LIB_ELF_H

#include <elf.h>

#include "lib.h"

#define ELFMAG "\177ELF"
#define SELFMAG 4

extern void err_quit(const char *msg);

void load_elf_binary(const char *buf);

#endif
