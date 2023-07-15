#include <stdlib.h>
#include <time.h>

int random_between(int min, int max) {

	time_t t;
	unsigned int res = time(&t);
	srand(res);
	return rand() % (max-min+1) + min;
}