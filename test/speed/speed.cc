#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <time.h>

int main(int argc, char **argv) {
	thread_local int B;
	clock_t start,finish;
	printf("Please enter: ");
	int a[1];
	scanf("%d", a);
	int a1 = a[0];
	float A = 1.00001;
	start=clock();
	B = 0;
	for(int i = 0; i < a1; i++) {
		B |= i*i*i;
		A += (float) i / 2;
	}
	finish = clock();
	printf("\nA: number %f", A);
	printf("\nB : %d", B);
	printf("\ntime cost: %ld.\n", (finish-start));
	return 0;
}
