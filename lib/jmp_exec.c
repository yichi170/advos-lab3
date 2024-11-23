#include "jmp_exec.h"
#include "lib.h"

void jump_exec(char* stack, void* entry) {
  printf("jump to entry point: %#lx\n", (uintptr_t)entry);

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
		: : "r" (stack), "r" (entry)
	);

	err_quit("Should not reach here");
}