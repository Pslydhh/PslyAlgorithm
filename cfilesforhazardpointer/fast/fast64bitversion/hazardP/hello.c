#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

#define	num	1

int* pointer;
void* kkk(void* k) {
	printf("EEE\n");
	sleep(5);
//	#ifdef ALLOC
	pointer = malloc(65536 * sizeof(int));
//	#endif	
	sleep(5);	
	return 0;
}

int main() {
	if(pointer == NULL)
		printf("hello, world\n");
	sleep(5);
	pthread_t pid[num];
	for(int i = 0; i < num; ++i)
		pthread_create(&pid[i], NULL, kkk, NULL);
	for(int i = 0; i < num; ++i)
		pthread_join(pid[i], NULL);
	printf("%ld\n", sizeof(pointer));
	#ifdef ALLOC
	free(pointer);
	#endif
	printf("%p\n", pointer);
	sleep(5);	
	return 0;
}
