#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SIZE 4096 * 1000

char a[SIZE];

int main(int argc, char **argv) {
	if (argc != 2) {
		printf("Usage: ./program <arg>\n");
		return 1;
	}
	int mode = atoi(argv[1]);
	if (mode == 1) {
		for (int i = 0; i < SIZE; i++) {
			a[i] = i;
		}
	} else if (mode == 2) {
		for (int j = 0; j < 4096; j++) {
			for (int i = j; i < SIZE; i += 4096) {
				a[i] = i;
			}
		}
	} else if (mode == 3) {
		srand(time(NULL));
		for (int i = 0; i < SIZE; i++) {
			a[rand() % SIZE] = i;
		}
	}
	printf("mode: %d\n", mode);
	return 0;
}
