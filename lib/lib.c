#include "lib.h"

void err_quit(const char *msg) {
	perror(msg);
	exit(EXIT_FAILURE);
}