#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

#define	N		100
int TIMES_;
typedef struct Handle_t {
	// 本地值，用于被Value索引, handle下标 + 版本号
	long value[2]; // __attribute__((aligned(128))); // value[2]， 两份， 因为要准确地确认数据的值。
	long version; // __attribute__((aligned(128))); // 版本号，用于确认值， handle[index].[version] 为我们索引的值。

	long wrapper; // 左边最高位1代表当前有悬挂请求,0代表无请求，右边16位作为此请求的版本号(也就是version)。

	int index; // 下一个将要协助的线程
	int steps; // steps fastpath / one help slowpath
} Handle_t; // __attribute__((aligned(128)));

pthread_t pid_[N];
Handle_t handle[N];
int ints[N * 1000000];

/*
 * value:64位：
   最右15位: 作为指向的handle + index:       PSLY_VALUE_INDEX
   接着16位: 作为该handle[index]的version;   PSLY_VALUE_VERSION
 */
long Value;
#define 	PSLY_VALUE_INDEX	((((long)1) << 15) - 1)
#define		PSLY_VALUE_VERSION	(((((long)1) << 31) - 1) ^ ((((long)1) << 15) - 1))
#define		PSLY_VALUE_INDEX2	(((((long)1) << 46) - 1) ^ ((((long)1) << 31) - 1))
#define		PSLY_VALUE_VERSION2  (((((long)1) << 62) - 1) ^ ((((long)1) << 46) - 1))
#define 	PSLY_MAX_POSITIVE	((((long)1) << 62) - 1)
#define 	PSLY_INTS			((((long)1) << 31) - 1)

long INDEX(long v) {
	return v & PSLY_VALUE_INDEX;
}

long INDEX2(long v) {
	return (v & PSLY_VALUE_INDEX2) >> 31;
}

long VERSION(long v) {
	return (v & PSLY_VALUE_VERSION) >> 15;
}

long VERSION2(long v) {
	return (v & PSLY_VALUE_VERSION2) >> 46;
}

bool CAS_VALUE(long cmp, long new) {
	return __sync_bool_compare_and_swap(&Value, cmp, new);
}

long nextValue(long value) {
	long result = INDEX2(value) | (((VERSION2(value) + 1) << 15) & PSLY_VALUE_VERSION);
	if(result < 0)
		printf("It's negative %ld\n", result);
	return result;
}

int getAndIncrement(int threadId) {
	Handle_t* meHandle = handle + threadId;
	int count = 1;int kkk = 0;
	for(;;) {if(++kkk > 100000) printf("%d herere\n", kkk);
		long Value_ = Value;
		if(Value_ >= 0) {
			long value = handle[ INDEX(Value_) ].value[ VERSION(Value_) & 1];
			meHandle->value[(meHandle->version + 1) & 1] = value + 1;
			if(CAS_VALUE(Value_, threadId | (((meHandle->version + 1)) << 15))) {
				meHandle->version = (meHandle->version + 1) & ((((long)1) << 16) - 1);
				if((++meHandle->steps) & 7 == 0) {
					help(meHandle->index % N);
					++meHandle->index;
				}
				return value;
			}
		} else {
			helpTransfer(Value_);
		}
		if(--count == 0)
			break;
	}
	meHandle->wrapper = (((long)1) << 63) | ((meHandle->version) << 46);
	//printf("%ld %ld wrapper version\n", VERSION2(meHandle->wrapper), meHandle->version);
	__sync_synchronize();
	help(threadId);
	meHandle->wrapper |= ((long)1) << 62;
	__sync_synchronize();
	meHandle->version = (meHandle->version + 1) & ((((long)1) << 16) - 1);
	long result = meHandle->value[(meHandle->version) & 1] -1;
	//printf("%ld value 1 %ld\n", result = meHandle->value[(meHandle->version) & 1] -1, meHandle->version);
	//sleep(1);
	//printf("%ld value 2 %ld\n", result = meHandle->value[(meHandle->version) & 1] -1, meHandle->version);
	return result;	
}

void help(int threadId) {
	Handle_t* handle_ = handle + threadId; 
	long wrapper = handle_->wrapper;
    if(wrapper & (((long) 1) << 62))
        return;
	if(wrapper < 0) {
		long version = VERSION2(wrapper);
		long value = Value;	
		long wrapper_;int kkk = 0;
		while(wrapper == (wrapper_ = handle_->wrapper)) { if(++kkk > 100000) printf("%d %ld\n", kkk, value);
			if(value >= 0) {
				long theValue = value | (((long)1) << 63) | ((version) << 46) | (((long) threadId) << 31);
				if(!CAS_VALUE(value, theValue)) {
					value = Value;
					continue;
				}
				value = theValue;	
			}
			helpTransfer(value);
			value = Value;	
		}
		if(version != VERSION2(wrapper_))
			return; 
		wrapper = wrapper_;
	}
	helpValueTransfer(handle_, wrapper);	
}

void helpTransfer(long value) {
	Handle_t* theHandle = handle + INDEX2(value);
	long version = VERSION2(value);
	long wrapper = theHandle->wrapper;	
	if((wrapper & (((long) 1) << 62)) || version != VERSION2(wrapper))
		return;
	if(wrapper < 0) {
		long localWrap;
		long localWrap2; 
		if((localWrap2 = __sync_val_compare_and_swap(&theHandle->wrapper, wrapper, localWrap = ((wrapper | (value & PSLY_INTS)) & PSLY_MAX_POSITIVE))) == wrapper)
			wrapper = localWrap;
		else {
			wrapper = localWrap2;
			if((wrapper & (((long) 1) << 62)) || version != VERSION2(wrapper))
				return;
		}
	}
	helpValueTransfer(theHandle, wrapper);
}

void helpValueTransfer(Handle_t* handle_, long wrapper) {
    long value2 = handle[INDEX(wrapper)].value[VERSION(wrapper) & 1] + 1;
	long value = Value;
//	printf("%here\n");
    if((value & PSLY_INTS) != (wrapper & PSLY_INTS) || VERSION2(value) != VERSION2(wrapper))
        return;
//	printf("here22222222\n");
	long ver = (VERSION2(wrapper) + 1) &  ((((long)1) << 16) - 1);
    long local;
    while((local = handle_->value[ver & 1]) < value2) 
    	__sync_bool_compare_and_swap(&handle_->value[ver & 1], local, value2);
	if(value == Value)
		CAS_VALUE(value, nextValue(value));
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
	for(int j = 0; j < TIMES_; ++j)
		ints[getAndIncrement((int) argv)] = 1;
	return NULL;	
}

int main(int argc, char** argv) {
	TIMES_ = atoi(argv[1]);
	int errTimes = 0;
	for(;;) {
    struct timeval start;
    gettimeofday(&start,NULL);
	Value = 0;
	for(int i = 0; i < N; ++i) {
		handle[i].wrapper |= ((long)1) << 62;
		handle[i].version = 0;
		handle[i].value[0] = 0;
		handle[i].value[1] = 0;
		int error;
		if((error = pthread_create(&pid_[i], NULL, routine, (void*) i))) {
			printf("%d can't create thread, errors: %d\n", i, error);
			fflush(stdout);
			exit(1);
		}
	}	
	for(int i = 0; i < N; ++i) {
		int error;
		if((error = pthread_join(pid_[i], NULL))) {
			printf("thread %d join errors: %d\n", i, error);
			fflush(stdout);
			exit(1);
		}	
	}	
	for(int j = 0; j < N * TIMES_; ++j) {
		if(ints[j] != 1) {
//			printf("ints[%d] == %d  wrong!\n", j, ints[j]);
			++errTimes;
		}
		ints[j] = 0;
	}
	printf("\n%d times errors\n", errTimes);
	struct timeval end;
	gettimeofday(&end, NULL);
	float time_use= (end.tv_sec-start.tv_sec)+(end.tv_usec-start.tv_usec) / 1000000.0;//微秒
	printf("time_use is %f\n\n", time_use);
	sleep(1);
	}
	return 1;
}

