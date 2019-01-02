#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>

#define NUMOFTHREAD		(1 << 16)	

/* for self */
//位移下标
#define IDXNUM	 16  //xuyao
#define IDXBIT  ((1 << IDXNUM) - 1)  //xuyao
//数组下标
#define ARRNUM    4             //xuyao
#define ARRAYNUM (1 << ARRNUM)     
#define ARRAYBITS (ARRAYNUM-1)
#define ARRBIT  (((1 << ARRNUM) - 1) << IDXNUM) //xuyao
#define ARRBITR ((1 << ARRNUM) - 1)
// 联合下标
#define ARRIDXBIT	(ARRBIT | IDXBIT)  //xuyao
/* for self */

/* for next 构造链表时使用*/
//位移下标
#define NEXTIDXNUM	IDXNUM
#define NEXTIDXBIT	IDXBIT
//尾节点bit
#define NEXTTAILNUM	1
#define NEXTTAILBIT	(((1 << NEXTTAILNUM) - 1) << NEXTIDXNUM)
//版本号记录
#define NEXTVERSIONNUM	(32 - NEXTTAILNUM - NEXTIDXNUM)
#define NEXTVERSIONBIT	((~0)^(NEXTTAILBIT | NEXTIDXBIT))
#define NEXTVERSIONONE	(1 + (NEXTTAILBIT | NEXTIDXBIT)) 
/* for next */

/* for tail */
#define TAILIDXNUM	IDXNUM
#define TAILIDXBIT	IDXBIT
//版本号记录
#define TAILVERSIONNUM 	(32 - TAILIDXNUM)
#define TAILVERSIONBIT	((~0)^TAILIDXBIT)
#define TAILVERSIONONE 	(1 + TAILIDXBIT)

/* for head */
#define HEADIDXNUM	IDXNUM
#define HEADIDXBIT	IDXBIT
//版本号记录
#define HEADVERSIONNUM	(32 - HEADIDXNUM)
#define HEADVERSIONBIT	((~0)^HEADIDXBIT)
#define HEADVERSIONONE	(1 + HEADIDXBIT)
/* for head */
/* 构造链表时使用 */

//pointer指向被记录的地址(不能被free)
typedef struct Record {
    int next;
    int self;

    long nextRecord;
    void* pointer;
} Record;

typedef struct RecordQueue {
    int head;
    int tail;
} RecordQueue;

Record* records[ARRAYNUM];
RecordQueue recordQueues[ARRAYNUM];
int recordTake = 0;

Record* idx_record(int index) {
    return records[(index & ARRBIT) >> IDXNUM] + (index & IDXBIT);
}

Record* get_record() {
    for(int i = 0; i < ARRAYNUM; ++i) {
        int array = __sync_fetch_and_add(&recordTake, 1) & ARRAYBITS;
        RecordQueue* queue = recordQueues + array;
        Record* arr = records[array];
        for(;;){
            int headIndex = queue->head;
            int indexHead = headIndex & HEADIDXBIT;
            Record* head = arr + indexHead;
            int tailIndex = queue->tail;
            int indexTail = tailIndex & TAILIDXBIT;
            int nextIndex = head->next;

            if(headIndex == queue->head) {
                if(indexHead == indexTail){
                    if((nextIndex & NEXTTAILBIT) == NEXTTAILBIT)
                        break;
                    __sync_bool_compare_and_swap(&queue->tail, tailIndex, (((tailIndex & TAILVERSIONBIT) + TAILVERSIONONE ) & TAILVERSIONBIT)|(nextIndex & TAILIDXBIT));    
                } else {
                    if(__sync_bool_compare_and_swap(&queue->head, headIndex, (((headIndex & HEADVERSIONBIT) + HEADVERSIONONE) & HEADVERSIONBIT)|(nextIndex & HEADIDXBIT))) {
                        return head;
                    }
                }
            }
        }
    }
	
   	for(int i = 0; i < ARRAYNUM; ++i) {
        int array = i;
        RecordQueue* queue = recordQueues + array;
        Record* arr = records[array];
        for(;;){
            int headIndex = queue->head;
            int indexHead = headIndex & HEADIDXBIT;
            Record* head = arr + indexHead;
            int tailIndex = queue->tail;
            int indexTail = tailIndex & TAILIDXBIT;

            int nextIndex = head->next;

            if(headIndex == queue->head) {
                if(indexHead == indexTail){
                    if((nextIndex & NEXTTAILBIT) == NEXTTAILBIT)
                        break;
                    __sync_bool_compare_and_swap(&queue->tail, tailIndex, (((tailIndex & TAILVERSIONBIT) + TAILVERSIONONE ) & TAILVERSIONBIT)|(nextIndex & TAILIDXBIT));    
                } else {
                    if(__sync_bool_compare_and_swap(&queue->head, headIndex, (((headIndex & HEADVERSIONBIT) + HEADVERSIONONE) & HEADVERSIONBIT)|(nextIndex & HEADIDXBIT))) {
                        return head;
                    }
                }
            }

        }
    }
	return NULL;
}
	
void return_record(Record* record) {
    record->next |= NEXTTAILBIT;
    int self = record->self;
    int array = (self >> IDXNUM) & ARRBITR;
    Record* arr = records[array];
    RecordQueue* queue = recordQueues + array;
    for(;;) {
        int tailIndex = queue->tail;
        int indexTail = tailIndex & TAILIDXBIT;
        Record* tail = arr + indexTail;
        int nextIndex = tail->next;

        if(tailIndex == queue->tail){
            if((nextIndex & NEXTTAILBIT) == NEXTTAILBIT) {
                if(__sync_bool_compare_and_swap(&tail->next, nextIndex, (((nextIndex & NEXTVERSIONBIT) + NEXTVERSIONONE) & NEXTVERSIONBIT)|(self & NEXTIDXBIT))){
                    __sync_bool_compare_and_swap(&queue->tail, tailIndex, (((tailIndex & TAILVERSIONBIT) + TAILVERSIONONE) & TAILVERSIONBIT)|(self & TAILIDXBIT));
                    return;
                }
            } else {
                __sync_bool_compare_and_swap(&queue->tail, tailIndex, (((tailIndex & TAILVERSIONBIT) + TAILVERSIONONE) & TAILVERSIONBIT)|(nextIndex & TAILIDXBIT));
            }
        }
    }
}

#define IDXNUM_		IDXNUM
#define IDXBIT_		((1 << IDXNUM_) - 1)	

#define ARRNUM_		ARRNUM
#define ARRBIT_		(((1 << ARRNUM_) - 1) << IDXNUM_)
#define ARRIDXBIT_	(ARRBIT_ | IDXBIT_)

#define NEXTB          17
#define NEXTBITS       ((NODEONE - 1) ^ ((NODEONE - 1) >> NEXTB))
#define NEXTONE		   (((NEXTBITS - 1) & NEXTBITS) ^ NEXTBITS)
#define NEXTTT         (NEXTBITS | ARRIDXBIT_)

#define NODEB          17
#define NODEBITS       ((REFONE -1 ) ^ ((REFONE - 1) >> NODEB))
#define NODEONE        (((NODEBITS - 1) & NODEBITS) ^ NODEBITS)

#define REFCB          (10)
#define REFCBITS       (((long)(-1)) << (64 - REFCB))
#define REFONE         (((long)1) << (64 - REFCB))
#define DELETED        0x0000000000000000
#define RECBW          (64 - REFCB)

#define RESTART		   2
#define KEEPPREV       1
#define NONE		   0

#define RECORD		   0x00000004
#define SEARCH         0x00000002
#define REMOVE         0x00000001 

#define LISTNUM		   512
//pointer指向将来被free的地址
typedef struct Node {
    //资源管理数据
    int next;
    int self;
    
    //功能数据
    void* nextNode;
    void* pointer;
} Node;

typedef struct NodeQueue {
    int head;
    int tail;
} NodeQueue;

typedef struct RecordList {
    Record* head;
	Record* tail;
} RecordList;

typedef struct RecordMap {
    RecordList lists[LISTNUM];
} RecordMap;

RecordMap map;

long nxtAddrV(long old, Record* replace) {
	return (old & REFCBITS) | (old & NODEBITS) | ((old + NEXTONE) & NEXTBITS) | ((replace->self & ARRBIT_) | (replace->self & IDXBIT_));
}

long nxtRecord(long old) {
//	printf("%ld\n", NODEONE); //sleep(5);
	return REFONE | ((old + NODEONE) & NODEBITS) | (old & NEXTBITS) | (old & ARRIDXBIT_);
}

long unNxtRecord(long old) {
	return 0 | ((old - NODEONE) & NODEBITS) | (old & NEXTBITS) | (old & ARRIDXBIT_);
}

long plusRecord(long old) {
	return ((old + REFONE) & REFCBITS) | (old & NODEBITS) | (old & NEXTBITS) | (old & ARRIDXBIT_);
}

long nxtAddr(long old, Record* replace) {
	return (old & ~ARRIDXBIT_) | ((replace->self & ARRBIT_) | (replace->self & IDXBIT_));
}

long newNext(long old, Record* replace) {
	return ((old + REFONE) & REFCBITS) | ((old + NODEONE) & NODEBITS) | (old & NEXTBITS) | (replace->self & ARRIDXBIT_);
}

int psly_handle_records(RecordList* list, void* pointer, int flag) {
	long key = (long) pointer;
	Record* head = list->head;
	Record* tail = list->tail;
	Record* my;
	int removed = 0;
	// Traverse the ordered list
	Record* prev;		
	Restart:	
	prev = head;
	long prevNext = prev->nextRecord;
	//printf("%d\n", prevNext);
	//sleep(5);
	Record* curr = idx_record(prevNext);
	long currNext;
	Record* found = NULL;
	long foundNext;
	for(;;){
		
		// keep logical prev, and look at remaining record
		KeepPrev:
		currNext = curr->nextRecord;
		void* currPointer = curr->pointer;
		long New = prev->nextRecord;

		// 1: if the curr->pointer is not "Really linked pointer", the prev must has been deleted or attached
		/*if((prevNext & ~REFCBITS) != (New & ~REFCBITS) || (New & REFCBITS) == DELETED)
			goto Restart;
		*/
		if((prevNext & NODEBITS) != (New & NODEBITS) || (New & REFCBITS) == DELETED)
			goto Restart;
		if((prevNext & NEXTTT) != (New & NEXTTT)) {
           	JUSTNEXTCHANGE: 
			prevNext = New;
            curr = idx_record(prevNext);
			found = NULL;
            goto KeepPrev;
        }
		prevNext = New;
		long currKey = (long) currPointer;
		if(curr == tail || currKey > key) {
			//printf("tail\n");
			if(found != NULL) {
				//printf("not NULL\n");
				// SEARCH
				if(flag == SEARCH) 
					return ((foundNext & REFCBITS) >> RECBW) & ((1 << REFCB) - 1) ;
				// process the found record
				for(;;) {
					//printf("found\n");
					New = found->nextRecord;
					if((foundNext & NODEBITS) != (New & NODEBITS)) {
						if(flag == REMOVE)
							return 0;
						//goto Restart;
						New = prev->nextRecord;
                        if((prevNext & NODEBITS) != (New & NODEBITS) || (New & REFCBITS) == DELETED)
                            goto Restart;
                        goto JUSTNEXTCHANGE;
					}
					foundNext = New;
					//curr = idx_record(foundNext);
					if((foundNext & REFCBITS) == DELETED) {
						curr = idx_record(foundNext);	
						// remove the deleted records
						CasRemove:
						if(__sync_bool_compare_and_swap(&prev->nextRecord, prevNext, New = nxtAddrV(prevNext, curr))) {
							// remove records succes!!!
							// return the records to respository
							long local = prevNext;
							prevNext = New;
							int STEP = NONE;
							do {	
								Record* first = idx_record(local);
								local = first->nextRecord;
								// return the record
								first->pointer = NULL;
								return_record(first);
								// RECORD	
								if(flag == RECORD) {
									New = prev->nextRecord;
									// prev has been deleted, so we will restart
									if((prevNext & NODEBITS) != (New & NODEBITS) || (New & REFCBITS) == DELETED) 
										STEP = (STEP < RESTART ? RESTART : STEP);
									// prev is still in list, just Its next has changed	
   			                    	if((prevNext & NEXTTT) != (New & NEXTTT)) 
										STEP = (STEP < KEEPPREV ? KEEPPREV : STEP);
									// maybe prev's ref has changed...never mind about it.
									prevNext = New;
								}	
							} while((local & ARRIDXBIT_) != (curr->self & ARRIDXBIT_));
							if(flag == REMOVE)
								return 0;
                            /*
							 * records has been return have all been returned
                             */
							if(STEP == RESTART)
								goto Restart;
							if(STEP == KEEPPREV) 
								goto JUSTNEXTCHANGE; 

							//my->nextRecord = nxtAddr(my->nextRecord, curr);
							// append our new record attach to prev, in front of curr
							long localN; 
							CasAppend:
					        my = get_record();
							New = prev->nextRecord;
                            if((prevNext & NODEBITS) != (New & NODEBITS) || (New & REFCBITS) == DELETED) {
                               	return_record(my); 
								goto Restart;
							}
                            if((prevNext & NEXTTT) != (New & NEXTTT)) {
								return_record(my);
                                goto JUSTNEXTCHANGE;
							}
							localN = my->nextRecord;
							my->pointer = pointer;
							my->nextRecord = newNext(my->nextRecord, curr);
							//printf("here\n");
							if(__sync_bool_compare_and_swap(&prev->nextRecord, prevNext, nxtAddrV(prevNext, my))) 
								return 1;
                            // append our record failed.....
							my->pointer = NULL;	
							my->nextRecord = localN;
							return_record(my);
							New = prev->nextRecord;
							if((prevNext & NODEBITS) != (New & NODEBITS) || (New & REFCBITS) == DELETED)
            					goto Restart;
							if((prevNext & NEXTTT) != (New & NEXTTT)) 
								goto JUSTNEXTCHANGE;
							// prev just change ref(Cause our append failed), so we append our record again  
							prevNext = New;
							goto CasAppend;		
						}
						New = prev->nextRecord;
                        if((prevNext & NODEBITS) != (New & NODEBITS) || (New & REFCBITS) == DELETED)
                            goto Restart;
                        if((prevNext & NEXTTT) != (New & NEXTTT)) 
                            goto JUSTNEXTCHANGE;
						// prev just change ref(Cause our remove failed), so we remove the deleted record again 
						prevNext = New;
                        goto CasRemove;	 	
					} else {
						if(flag == REMOVE) {
							long refNum;
							if((refNum = (__sync_sub_and_fetch(&found->nextRecord, REFONE) & REFCBITS)) != DELETED)
								return (refNum >> RECBW) & ((1 << REFCB) -1);
							removed = 1;
							//return (refNum >> RECBW) & (( 1 << REFCB) - 1);
						} else {
							/*if(pointer != found->pointer) {
								New = prev->nextRecord;
 			                    if((prevNext & NODEBITS) != (New & NODEBITS) || (New & REFCBITS) == DELETED)
                            		goto Restart;
                        		goto JUSTNEXTCHANGE;
							} */
							long refNum; 
							if(__sync_bool_compare_and_swap(&found->nextRecord, foundNext, refNum = plusRecord(foundNext)))
								return ((refNum & REFCBITS) >> RECBW) & ((1 << REFCB) -1); 
						}	
					}
				}				
			}
			/*
			 * without this record
			 */
			//printf("not found\n");
			// SEARCH & REMOVE
			if(flag == SEARCH)
				return 0;
			// 1:maybe some nonadjacent in else...
			// 2:but all adjacents must be in else	
			if((prevNext & ARRIDXBIT_) != (curr->self & ARRIDXBIT_)) {
				//printf("to remove\n");
				goto CasRemove;		
			} else { //
				if(removed)
					return 0;	
				goto CasAppend;	
			}
		} else {
			// logical
			New = curr->nextRecord;
			if((currNext & NODEBITS) != (New & NODEBITS)) {
				// goto Restart;
				New = prev->nextRecord;
                if((prevNext & NODEBITS) != (New & NODEBITS) || (New & REFCBITS) == DELETED)
                    goto Restart;
                goto JUSTNEXTCHANGE;
			}
			currNext = New;
			if((currNext & REFCBITS) == DELETED) {
				curr = idx_record(currNext);
				continue;
			}
			if(currKey != key) {
				prev = curr;
				prevNext = currNext;
			} else {
				if(removed)
					return 0;
				found = curr;
				foundNext = currNext;
			}
			curr = idx_record(currNext);
		}	 	
	}
}

int psly_record(void* pointer) {
	long key = (long) pointer;
	RecordList* list = &map.lists[key & (LISTNUM-1)];
	//printf("%d\n", LISTNUM - 1);
	//sleep(5);
	return psly_handle_records(list, pointer, RECORD);	
}

int psly_remove(void* pointer) {
	long key =(long) pointer;
	RecordList* list = &map.lists[key & (LISTNUM-1)];
	return psly_handle_records(list, pointer, REMOVE);
}

int psly_search(void* pointer) {
    long key =(long) pointer;
    RecordList* list = &map.lists[key & (LISTNUM-1)];
	//printf("%d\n", key & INDEXBITS);
	return psly_handle_records(list, pointer, SEARCH);
}





pthread_barrier_t b;


int count;

int count2;
int noCount;
int flagStart;
#define 	numOfItems		  1024
int refCount[numOfItems];  
int recordremove;
pthread_barrier_t barrier;
pthread_mutex_t	  mutex;
int totalOperation;
pthread_barrier_t barrier;
struct timeval start;
void* recordremoveRoutine(void *argv) {
	int seed = (int) argv;
	srand(seed);
	int key = rand() & (numOfItems - 1);
	for(int i = 0; i < recordremove; ++i) {
	//		 int key = rand() & (numOfItems - 1);
			int refc = refCount[key];
//			gettimeofday(&start2, NULL);
//			if((start2.tv_usec & 111) >= 0) {
				for(;;) {
				if(refc < ((1 << REFCB) - 1)) { 
					if( __sync_bool_compare_and_swap(refCount + key, refc, refc + 1)) {
						psly_record(key * (LISTNUM)/* * INDEXBITS*/);
						__sync_fetch_and_add(&totalOperation, 1);
						break;
					}
					refc = refCount[key];	
				} else break;
				}
//			}
		   key = (key + 191) & (numOfItems - 1);	
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
			if(ref <= 0)
				break;
			if(__sync_bool_compare_and_swap(refCount + i, ref, ref - 1)) {
				psly_remove(i * (LISTNUM));
				__sync_fetch_and_add(&totalOperation, 1);	
			}	
		}
		i = (i+1) & (numOfItems - 1);
	} 	
	return NULL;	
}

int main(int argc, char** argv) {
/*    printf("NEXTB: %ld\n", NEXTB); 
    printf("NEXTBITS: %ld\n", NEXTBITS); 
    printf("NEXTONE: %ld\n", NEXTONE); 
    printf("NEXTTT: %ld\n", NEXTTT); 
    printf("NODEB: %ld\n", NODEB); 
    printf("NODEBITS: %ld\n", NODEBITS); 
    printf("NODEONE: %ld\n", NODEONE); 
    printf("REFCB: %ld\n", REFCB); 
    printf("REFCBITS: %ld\n", REFCBITS); 
    printf("REFONE: %ld\n", REFONE); 
    printf("DELETED: %ld\n", DELETED); 
    printf("RECBW: %ld\n", RECBW); 
*/
	printf("IDXNUM: %ld\n", IDXNUM); 
    printf("IDXBIT: %ld\n", IDXBIT); 
    printf("ARRNUM: %ld\n", ARRNUM); 
    printf("ARRAYNUM: %ld\n", ARRAYNUM); 
    printf("ARRAYBITS: %ld\n", ARRAYBITS); 
	printf("ARRBIT: %ld\n", ARRBIT); 
    printf("ARRBITR: %ld\n", ARRBITR); 
    printf("ARRIDXBIT: %ld\n", ARRIDXBIT); 
    printf("NEXTIDXNUM: %ld\n", NEXTIDXNUM); 
    printf("NEXTIDXBIT: %ld\n", NEXTIDXBIT); 
	
	printf("NEXTTAILNUM: %ld\n", NEXTTAILNUM); 
	printf("NEXTTAILBIT: %ld\n", NEXTTAILBIT); 
    printf("NEXTVERSIONNUM: %ld\n", NEXTVERSIONNUM); 
    printf("NEXTVERSIONBIT: %ld\n", NEXTVERSIONBIT); 
    printf("NEXTVERSIONONE: %ld\n", NEXTVERSIONONE); 
    printf("TAILIDXNUM: %ld\n", TAILIDXNUM); 
	printf("TAILIDXBIT: %ld\n", TAILIDXBIT); 
	
    printf("TAILVERSIONNUM: %ld\n", TAILVERSIONNUM); 
    printf("TAILVERSIONBIT: %ld\n", TAILVERSIONBIT); 
	printf("TAILVERSIONONE: %ld\n", TAILVERSIONONE); 
    printf("HEADIDXNUM: %ld\n", HEADIDXNUM); 
	printf("HEADIDXBIT: %ld\n", HEADIDXBIT); 
    printf("HEADVERSIONNUM: %ld\n", HEADVERSIONNUM); 
    printf("HEADVERSIONBIT: %ld\n", HEADVERSIONBIT); 
	printf("HEADVERSIONONE: %ld\n", HEADVERSIONONE); 		
    //sleep(5);

	if(argc != 3)
		return 1;
	int num = atoi(argv[1]);
	recordremove = atoi(argv[2]);	
	float time_use=0;
    struct timeval end;

	pthread_t pids[NUMOFTHREAD];

	pthread_barrier_init(&barrier, NULL, num + 1);
	pthread_mutex_init(&mutex, NULL);

	for(int i = 0; i < (1 << ARRNUM); ++i){
        Record* record;
        records[i] = record = (Record*) malloc((1 << IDXNUM) * sizeof(Record));
		memset(record, 0, (1 << IDXNUM) * sizeof(Record));
        for(int j = 0; j < (1 << IDXNUM) - 1; ++j){
            record->self = (i << IDXNUM) | j;
            record->next = j+1;
            //record->nextRecord = 0;
            //record->pointer = NULL;
            record += 1;
        }
        record->self = (i << IDXNUM) | ((1 << IDXNUM) - 1);
        record->next = NEXTTAILBIT;
        //record->nextRecord = 0;
        //record->pointer = NULL;
        recordQueues[i].head = 0;
        recordQueues[i].tail = (1 << IDXNUM) - 1;
    }
	
	for(int i = 0; i < LISTNUM; ++i) {
		RecordList* list = &map.lists[i];
		Record* head = get_record();
		Record* tail = get_record();
		head->nextRecord = plusRecord(head->nextRecord);
		head->nextRecord = nxtAddr(head->nextRecord, tail);
//		printf("%d %d\n", i, head->nextRecord);
		list->head = head;
		list->tail = tail;	
	}
	printf("records: %d\n", sizeof(records));	
//	gettimeofday(&start, NULL);

	printf("\n\n\n");
	int errCount = 0;
	pthread_mutex_lock(&mutex);
	gettimeofday(&start, NULL);
	for(int i = 0; i < num; ++i)
        pthread_create(pids + i, NULL, recordremoveRoutine, (void*) i);
	pthread_barrier_wait(&barrier);
	for(int i = 0; i < numOfItems; ++i) {
		int re = psly_search(i * (LISTNUM));
//		printf("%d, %d, %d\n", refCount[i], re, i);
		if(i > 1000) break;
		if(refCount[i] != re) 
            ++errCount;
	}  
//	sleep(5);	
	pthread_mutex_unlock(&mutex);
	for(int i = 0; i < num; ++i)
		pthread_join(pids[i], NULL);		
	int total = num * recordremove;
	int add = 0;
	for(int i = 0; i < numOfItems; ++i) {
		int re;	
		if(refCount[i] != (re = psly_search(i * (LISTNUM)))) {
			++errCount;
		}
//		 printf("%d, %d\n", refCount[i], re);
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
//		printf("one\n");
		head = idx_record(head->nextRecord);
	}		
	}  
	printf("%d, %d, %d\n", LISTNUM, k, LISTNUM == k);
	return 0;	
}
