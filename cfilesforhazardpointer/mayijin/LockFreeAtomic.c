#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

#define	N		100
int TIMES;
typedef struct Handle_t {
	// 本地值，用于被Value索引, handle下标 + 版本号
	long value[2]; // __attribute__((aligned(128))); // value[2]， 两份， 因为要准确地确认数据的值。
	short version; // __attribute__((aligned(128))); // 版本号，用于确认值， handle[index].[version] 为我们索引的值。
} Handle_t; // __attribute__((aligned(128)));

pthread_t pid[N];
Handle_t handle[N];
int ints[N * 1000000];

/*
 * value:64位：
   最右10位: 作为指向的handle + index:       PSLY_VALUE_INDEX
   接着16位: 作为该handle[index]的version;   PSLY_VALUE_VERSION
 */
long Value;
#define 	PSLY_BIT 			16
#define 	PSLY_VALUE_INDEX	((1 << 16) - 1)
#define		PSLY_VALUE_VERSION	(((1 << 32) - 1) ^ ((1 << 16) - 1))

int INDEX(long v) {
	return v & PSLY_VALUE_INDEX;
}
int VERSION(long v) {
	return (v & PSLY_VALUE_VERSION) >> 16;
}
bool CAS_VALUE(long cmp, long new) {
	return __sync_bool_compare_and_swap(&Value, cmp, new);
}

int getAndIncrement(int threadId) {
	Handle_t* meHandle = handle + threadId;
	for(;;) {
		long Value_ = Value;
		long value = handle[ INDEX(Value_) ].value[ VERSION(Value_) & 1];
		if(Value_ != Value)
			continue;
		meHandle->value[(meHandle->version + 1) & 1] = value + 1;
		if(CAS_VALUE(Value_, threadId | ((meHandle->version + 1) << 16))) {
			meHandle->version = meHandle->version + 1;
			return value;
		}
	}
}

int getAndIncrement2(int threadId) {
	for(;;) {
		long value = Value;
		if(__sync_bool_compare_and_swap(&Value, value, value + 1))
			return value;
		__asm__("pause");
	}	
}
int getAndIncrement3(int threadId) {
	return __sync_fetch_and_add(&Value, 1);
}

void* routine(void* argv) {
	for(int j = 0; j < TIMES; ++j)
		ints[getAndIncrement((int) argv)] = 1;
	return NULL;	
}

int main(int argc, char** argv) {
	TIMES = atoi(argv[1]);
    struct timeval start;
    gettimeofday(&start,NULL);
	int errTimes = 0;
	for(int i = 0; i < N; ++i) {
		int error;
		if((error = pthread_create(&pid[i], NULL, routine, (void*) i))) {
			printf("%d can't create thread, errors: %d\n", i, error);
			fflush(stdout);
			exit(1);
		}
	}	
	for(int i = 0; i < N; ++i) {
		int error;
		if((error = pthread_join(pid[i], NULL))) {
			printf("thread %d join errors: %d\n", i, error);
			fflush(stdout);
			exit(1);
		}	
	}	
	for(int j = 0; j < N * TIMES; ++j) {
		if(ints[j] != 1) {
			printf("ints[%d] == %d  wrong!\n", j, ints[j]);
			++errTimes;
		}
	}
	printf("\n%d times errors\n", errTimes);
	struct timeval end;
	gettimeofday(&end, NULL);
	float time_use= (end.tv_sec-start.tv_sec)+(end.tv_usec-start.tv_usec) / 1000000.0;//微秒
	printf("time_use is %f\n\n", time_use);
	return 1;
}

