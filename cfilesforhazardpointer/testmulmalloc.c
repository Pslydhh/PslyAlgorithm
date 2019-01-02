#include<stdio.h>
#include<stdlib.h>
#include <pthread.h>
//#include <sys/syscall.h>
#include <semaphore.h>
//#include <sys/stat.h>
//#include <sys/types.h>
#include <fcntl.h>

#define		NUM		100

void *addr;
void *addr2 = 234;
sem_t mutex;

void* pfunc(void *arg){
	printf("malloc: %ld\n", pthread_self());
    addr = malloc(1 * sizeof(int));
	sem_post(&mutex);
    return 0;

}

void* pfunc2(void *arg){
	printf("HHHHHHHHHHHHHH\n");
	int val;
	sem_wait(&mutex);
  	int* intArray = (int*) addr;
    intArray[0] = 5;
    intArray[1] = 8;
    intArray[100] = 100;
    intArray[1000] = 1000;
	printf("free: %ld\n", pthread_self());
    printf("intArray: %d intArray[0]: %d intArray[1]: %d intArray[100]: %d intArray[1000]: %d\n\n", (int)intArray, intArray[0], intArray[1], intArray[100], intArray[1000]);
	free(intArray);
}

int main(int argc, char** argv){
	printf("main: %ld\n", pthread_self());	
	sem_init(&mutex, 0, 0);
	pthread_t pid, pid2;
	pthread_create(&pid, NULL, pfunc, NULL);
//	pthread_join(pid, NULL);
	pthread_create(&pid2, NULL, pfunc2, NULL);
	pthread_join(pid2, NULL);
	pthread_join(pid, NULL);
	printf("%ld\n", sizeof(void*));
	return 0;
}
