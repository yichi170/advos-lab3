#include <stdio.h>
#include <stdlib.h>


int main() {
  char *ptr = malloc(64 * sizeof(char)); 

  int i;
  for (i = 0; i < 32; i++) {
    ptr[i] = 'A' + i;
  }
  ptr[i] = '\0';

  printf("ptr: %s\n", ptr);

  free(ptr);

  return 0;
}
