#include <stdio.h>
#include <stdlib.h>
#include "hazard.h"

#define LISTNUM 512
DEFINE_RESOURCE_FOR_HAZARD(Record, 16, 16, 4, LISTNUM) 


pthread_barrier_t b;

#define 	numOfItems		  1024
int refCount[numOfItems];  
int recordremove;
pthread_barrier_t barrier;
pthread_mutex_t	  mutex;
int totalOperation;
pthread_barrier_t barrier;

void* recordremoveRoutine(void *argv) {
	int seed = (int) argv;
	srand(seed);
	int key = rand() & (numOfItems - 1);
	for(int i = 0; i < recordremove; ++i) {
			int refc = refCount[key];
			for(;;) {
				if(refc < ((1 << REFCB) - 1)) {
					if( __sync_bool_compare_and_swap(refCount + key, refc, refc + 1)) {
						psly_record(key * (LISTNUM-1));
						__sync_fetch_and_add(&totalOperation, 1);
						break;
					}
					refc = refCount[key];	
				} else break;
			}
		   key = (key + 1) & (numOfItems - 1);	
	}
	pthread_barrier_wait(&barrier);
	pthread_mutex_lock(&mutex);
	pthread_mutex_unlock(&mutex);
	//printf("%d has gone\n", pthread_self());
	int startKey = rand() & (numOfItems - 1);
	int j = 0;
	for(int i = startKey; j < numOfItems; ++j) {
		for(;;) {
			int ref = refCount[i];
			if(ref <= 197)
				break;
			if(__sync_bool_compare_and_swap(refCount + i, ref, ref - 1)) {
				psly_remove(i * (LISTNUM-1));
				__sync_fetch_and_add(&totalOperation, 1);	
			}	
		}
		i = (i+1) & (numOfItems - 1);
	} 	
	return NULL;	
}

#define 	NUMOFTHREAD		10240
int main(int argc, char** argv) {
	INIT_RESOURCE(Record, LISTNUM);
	if(argc != 3)
		return 1;
	int num = atoi(argv[1]);
	recordremove = atoi(argv[2]);	
	float time_use=0;
    struct timeval end;

	pthread_t pids[NUMOFTHREAD];

	pthread_barrier_init(&barrier, NULL, num + 1);
	pthread_mutex_init(&mutex, NULL);

	struct timeval start;
	gettimeofday(&start, NULL);

	printf("\n\n\n");
	int errCount = 0;
	pthread_mutex_lock(&mutex);
	gettimeofday(&start, NULL);
	for(int i = 0; i < num; ++i)
        pthread_create(pids + i, NULL, recordremoveRoutine, (void*) i);
	pthread_barrier_wait(&barrier);
	for(int i = 0; i < numOfItems; ++i) {
		int re = psly_search(i * (LISTNUM-1));
		if( i <= 1000)
			printf("%d, %d, %d\n", refCount[i], re, i);
		if(refCount[i] != re) 
            ++errCount;
	}
//	sleep(10);  
	pthread_mutex_unlock(&mutex);
	for(int i = 0; i < num; ++i)
		pthread_join(pids[i], NULL);		
	int total = num * recordremove;
	int add = 0;
	for(int i = 0; i < numOfItems; ++i) {
		int re;	
		if(refCount[i] != (re = psly_search(i * (LISTNUM-1)))) {
			++errCount;
		}
		if(i <= 1000) 
		 	printf("%d, %d\n", refCount[i], re);
	} 
	printf("err numbers: %d\n", errCount);
	printf("total: %d, adds: %d\n", total, add);
	printf("actual op: %d\n", totalOperation);
	gettimeofday(&end,NULL);
    time_use=(end.tv_sec-start.tv_sec)+(end.tv_usec-start.tv_usec) / 1000000.0;//微秒
	printf("total_use: %f\n\n", time_use);
	int k = 0;
	for(int i = 0; i < LISTNUM; ++i) {
		Record* head = map.lists[i].head;
		Record* tail = map.lists[i].tail;
		while(head != tail) {
			++k;
			head = idx_Record(head->nextRecord);
		}		
	}  
	printf("%d, %d, %d\n", LISTNUM, k, LISTNUM == k);
	return 0;	
}
