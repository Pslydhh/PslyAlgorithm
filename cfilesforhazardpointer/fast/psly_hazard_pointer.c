#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#include <pthread.h>

#define NUMOFTHREAD		10240	
#define NUMOFMAXPER     100000

/* for self */
//位移下标
#define IDXNUM	10
#define IDXBIT  ((1 << IDXNUM) - 1)
//数组下标
#define ARRNUM  2
#define ARRBIT  (((1 << ARRNUM) - 1) << IDXNUM)
#define ARRBITR ((1 << ARRNUM) - 1)
#define ARRONE  (1 << IDXNUM)
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
#define HEADIDXNUM	IDEXNUM
#define HEADIDXBIT	IDXBIT
//版本号记录
#define HEADVERSIONNUM	(32 - HEADIDXNUM)
#define HEADVERSIONBIT	((~0)^HEADIDXBIT)
#define HEADVERSIONONE	(1 + HEADIDXBIT)
/* for head */

#define ARRAYNUMb 2
#define ARRAYNUM  (1 << ARRAYNUMb)
#define ARRAYPERb 10
#define ARRAYBITS (ARRAYNUM - 1) 
#define ARRAYLEN  (1 << ARRAYPERb)
#define UNUSE          ARRAYLEN
#define INDEXBITS      (UNUSE - 1)
#define VERSIONBITS    (~((1 << (ARRAYNUMb + ARRAYPERb)) | ((1 << (ARRAYNUMb + ARRAYPERb))-1)))
#define VERSIONONES    (1 << (ARRAYNUMb + 1 +  ARRAYPERb))
#define ARRSHIFT       (ARRAYPERb + 1)
#define ARRBITS        ((ARRAYNUM - 1) << ARRSHIFT) 

#define ARRINDEXBITS   (((ARRAYNUM - 1) << ARRAYPERb) | (ARRAYLEN - 1))
#define RARRBITS	   ((ARRAYNUM - 1) << ARRAYPERb)
#define RIDXBITS	   (ARRAYLEN - 1)
#define INDEXSHIFT     0

#define REFCB		   10
#define REFCBITS	   ((-1) << (32 - REFCB))
#define REFONE         (1 << (32 - REFCB))
#define DELETED        0x00000000
#define RECBW          (32 - REFCB)

#define NODEB		   5
#define NODEBITS	   ((REFONE -1 ) ^ ((REFONE - 1) >> NODEB))
#define NODEONE		   (((NODEBITS - 1) & NODEBITS) ^ NODEBITS)

#define NEXTB          5
#define NEXTBITS       ((NODEONE - 1) ^ ((NODEONE - 1) >> NEXTB))
#define NEXTONE		   (((NEXTBITS - 1) & NEXTBITS) ^ NEXTBITS)

#define NEXTTT         (NEXTBITS | ARRINDEXBITS)

#define RESTART		   2
#define KEEPPREV       1
#define NONE		   0

#define RECORD		   0x00000004
#define SEARCH         0x00000002
#define REMOVE         0x00000001 

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

//pointer指向被记录的地址(不能被free)
typedef struct Record {
    int next;
    int self;

    int nextRecord;
    void* pointer;
} Record;

typedef struct RecordQueue {
    int head;
    int tail;
} RecordQueue;

typedef struct RecordList {
    Record* head;
	Record* tail;
} RecordList;

typedef struct RecordMap {
    RecordList lists[ARRAYLEN];
} RecordMap;

RecordMap map;
Record* records[ARRAYNUM];
RecordQueue recordQueues[ARRAYNUM];
int recordTake = 0;

Record* get_one_record() {
	for(int i = 0; i < ARRAYNUM; ++i) {
		int array = __sync_fetch_and_add(&recordTake, 1) & ARRAYBITS;
		RecordQueue* queue = recordQueues + array;
		Record* arr = records[array];
		int count = 0;	
		for(;;){
			if((++count) % 10000 == 0)
				printf("%d\n", count);
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
		int count = 0;
        for(;;){
			if((++count) % 10000 == 0)
                printf("%d\n", count);
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

void return_the_record(Record* record) {
	record->next |= NEXTTAILBIT;
	int self = record->self;
	int array = (self >> IDXNUM) & ARRBITR; 
	Record* arr = records[array];
	RecordQueue* queue = recordQueues + array;
	int count = 0;	
	for(;;) {
		if((++count) % 10000 == 0)
			printf("has loop %d\n", count);	
		int tailIndex = queue->tail;
		int indexTail = tailIndex & TAILIDXBIT;
		Record* tail = arr + indexTail;
		int nextIndex = tail->next;

		if(tailIndex == queue->tail){
			if((nextIndex & NEXTTAILBIT) == NEXTTAILBIT) {
			//	printf("dddddddddddddd\n");
				if(__sync_bool_compare_and_swap(&tail->next, nextIndex, (((nextIndex & NEXTVERSIONBIT) + NEXTVERSIONONE) & NEXTVERSIONBIT)|(self & NEXTIDXBIT))){
					__sync_bool_compare_and_swap(&queue->tail, tailIndex, (((tailIndex & TAILVERSIONBIT) + TAILVERSIONONE) & TAILVERSIONBIT)|(self & TAILIDXBIT));
					return;	
				} 
			} else {
				printf("%d\n", nextIndex & NEXTTAILBIT);
			//	printf("loop here %d\n", count); 
				if(!__sync_bool_compare_and_swap(&queue->tail, tailIndex, (((tailIndex & TAILVERSIONBIT) + TAILVERSIONONE) & TAILVERSIONBIT)|(nextIndex & TAILIDXBIT)))
					printf("false\n");
				else
					printf("true\n");
			}	
		}
	}	
}

Record* takeNext(int nextRecord) {
	return records[(nextRecord & RARRBITS) >> ARRAYPERb] + (nextRecord & RIDXBITS);
}

int nxtAddrV(int old, Record* replace) {
	return (old & REFCBITS) | (old & NODEBITS) | ((old + NEXTONE) & NEXTBITS) | ((replace->self & RARRBITS) | (replace->self & RIDXBITS));
}

int nxtRecord(int old) {
	return REFONE | ((old + NODEONE) & NODEBITS) | (old & NEXTBITS) | (old & ARRINDEXBITS);
}

int unNxtRecord(int old) {
	return 0 | ((old - NODEONE) & NODEBITS) | (old & NEXTBITS) | (old & ARRINDEXBITS);
}

int plusRecord(int old) {
	return ((old + REFONE) & REFCBITS) | (old & NODEBITS) | (old & NEXTBITS) | (old & ARRINDEXBITS);
}

int changeNext(int old) {
	return 0;
}

int nxtAddr(int old, Record* replace) {
	return (old & ~ARRINDEXBITS) | ((replace->self & RARRBITS) | (replace->self & RIDXBITS));
}

int newNext(int old, Record* replace) {
	return ((old + REFONE) & REFCBITS) | ((old + NODEONE) & NODEBITS) | (old & NEXTBITS) | (replace->self & ARRINDEXBITS);
}

void removeRecord(RecordList* list, long key, int version) {

}

int contract;

int psly_handle_records(RecordList* list, void* pointer, int flag) {
	int count = 0;
	long key = (long) pointer;
	Record* head = list->head;
	Record* tail = list->tail;
	Record* my;
	int removed = 0;
	// Traverse the ordered list
	Record* prev;		
	Restart:	
	prev = head;
	int prevNext = prev->nextRecord;
	void* prevValue = -1;
	Record* curr = takeNext(prevNext);
	int currNext;
	Record* found = NULL;
	int foundNext;
	for(;;){
		
		// keep logical prev, and look at remaining record
		KeepPrev:
		if((++count) % 10000 == 0)
			printf("has loop %d times\n", count);
		currNext = curr->nextRecord;
		void* currPointer = curr->pointer;
		int New = prev->nextRecord;

		// 1: if the curr->pointer is not "Really linked pointer", the prev must has been deleted or attached
		/*if((prevNext & ~REFCBITS) != (New & ~REFCBITS) || (New & REFCBITS) == DELETED)
			goto Restart;
		*/
		if((prevNext & NODEBITS) != (New & NODEBITS) || (New & REFCBITS) == DELETED)
			goto Restart;
		if((prevNext & NEXTTT) != (New & NEXTTT)) {
           	JUSTNEXTCHANGE: 
			prevNext = New;
            curr = takeNext(prevNext);
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
					if((++count) % 10000 == 0)
            printf("has loop %d times\n", count);
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
					if((foundNext & REFCBITS) == DELETED) {
						curr = takeNext(foundNext);
						// remove the deleted records
						CasRemove:
						if((++count) % 10000 == 0)
            printf("has loop %d times\n", count);
						if(__sync_bool_compare_and_swap(&prev->nextRecord, prevNext, New = nxtAddrV(prevNext, curr))) {
							// remove records succes!!!
							// return the records to respository
							int local = prevNext;
							prevNext = New;
							int STEP = NONE;
							do {	
								if((++count) % 10000 == 0)
            printf("has loop %d times\n", count);
								Record* first = takeNext(local);
								local = first->nextRecord;
								// return the record
								first->pointer = NULL;
								return_the_record(first);
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
							} while((local & ARRINDEXBITS) != (curr->self & ARRINDEXBITS));
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
							int localN; 
							CasAppend:
							if((++count) % 10000 == 0)
            printf("has loop %d times\n", count);
					        my = get_one_record();
							New = prev->nextRecord;
                            if((prevNext & NODEBITS) != (New & NODEBITS) || (New & REFCBITS) == DELETED) {
                               	return_the_record(my); 
								goto Restart;
							}
                            if((prevNext & NEXTTT) != (New & NEXTTT)) {
								return_the_record(my);
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
							return_the_record(my);
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
							int refNum;
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
							int refNum; 
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
			if((prevNext & ARRINDEXBITS) != (curr->self & ARRINDEXBITS)) {
				//printf("to remove\n");
				goto CasRemove;		
			} else { //
				//printf("to append\n");
				if(flag == REMOVE) {
					printf("the contract: %d,%d\n", flag == REMOVE, removed);
					printf("prevValue:%d, selfValue:%d, currValue:%d\n", prevValue, key, currKey);	
				}
				if((flag == REMOVE) != removed)
					__sync_add_and_fetch(&contract, 1);	
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
				curr = takeNext(currNext);
				continue;
			}
			if(currKey != key) {
				prev = curr;
				prevNext = currNext;
				prevValue = currKey;
			} else {
				if(removed)
					return 0;
				found = curr;
				foundNext = currNext;
			}
			curr = takeNext(currNext);
		}	 	
	}
}

int psly_record(void* pointer) {
	long key = (long) pointer;
	RecordList* list = &map.lists[key & INDEXBITS];
	//printf("%d\n", key & INDEXBITS);
	return psly_handle_records(list, pointer, RECORD);	
}

int psly_remove(void* pointer) {
	long key =(long) pointer;
	RecordList* list = &map.lists[key & INDEXBITS];
	return psly_handle_records(list, pointer, REMOVE);
}

int psly_search(void* pointer) {
    long key =(long) pointer;
    RecordList* list = &map.lists[key & INDEXBITS];
	return psly_handle_records(list, pointer, SEARCH);
}


Node* nodes[ARRAYNUM];
NodeQueue nodeQueues[ARRAYNUM];
int nodeTake = 0;


void return_the_node(Node* node) {
	node->next |= UNUSE;
	int self = node->self;
	int arrayIndex = (self & ARRBITS) >> ARRSHIFT;
	Node* arr = nodes[arrayIndex];
	NodeQueue* queue = nodeQueues + arrayIndex;
	
	for(;;) {
		int tailIndex = queue->tail;
		int indexTail = tailIndex & INDEXBITS;
		Node* tail = arr + indexTail;
		int nextIndex = tail->next;

		if(tailIndex == queue->tail){
			if((nextIndex & UNUSE) == UNUSE) {
				if(__sync_bool_compare_and_swap(&tail->next, nextIndex, (((nextIndex & ~UNUSE) + VERSIONONES) & VERSIONBITS)|(self & INDEXBITS))){
					__sync_bool_compare_and_swap(&queue->tail, tailIndex, ((tailIndex + VERSIONONES) & VERSIONBITS)|(self & INDEXBITS));
					return;	
				} 
			} else { 
				__sync_bool_compare_and_swap(&queue->tail, tailIndex, ((tailIndex + VERSIONONES) & VERSIONBITS)|(nextIndex & INDEXBITS));
			}	
		}
	}	
}

Node* get_one_node() {
	for(int i = 0; i < ARRAYNUM; ++i) {
		int array = __sync_fetch_and_add(&nodeTake, 1) & ARRAYBITS;
		NodeQueue* queue = nodeQueues + array;
		Node* arr = nodes[array];	
		for(;;){
			int headIndex = queue->head;
			int indexHead = headIndex & INDEXBITS;
			Node* head = arr + indexHead;
			int tailIndex = queue->tail;
			int indexTail = tailIndex & INDEXBITS;
			int nextIndex = head->next;

			if(headIndex == queue->head) {
				if(indexHead == indexTail){
					if((nextIndex & UNUSE) == UNUSE)
						break;
					__sync_bool_compare_and_swap(&queue->tail, tailIndex, ((tailIndex + VERSIONONES) & VERSIONBITS)|(nextIndex & INDEXBITS));	
				} else {
					if(__sync_bool_compare_and_swap(&queue->head, headIndex, ((headIndex + VERSIONONES) & VERSIONBITS)|(nextIndex & INDEXBITS))) {
						return head;
					}
				}
			}	
		}		
	}

	for(int i = 0; i < ARRAYNUM; ++i) {
        int array = i;
        NodeQueue* queue = nodeQueues + array;
        Node* arr = nodes[array];
        for(;;){
            int headIndex = queue->head;
            int indexHead = headIndex & INDEXBITS;
            Node* head = arr + indexHead;
            int tailIndex = queue->tail;
            int indexTail = tailIndex & INDEXBITS;
            
            int nextIndex = head->next;
            
            if(headIndex == queue->head) {
                if(indexHead == indexTail){
                    if((nextIndex & UNUSE) == UNUSE)
                        break;
                    __sync_bool_compare_and_swap(&queue->tail, tailIndex, ((tailIndex + VERSIONONES) & VERSIONBITS)|(nextIndex & INDEXBITS));
                } else {
                    if(__sync_bool_compare_and_swap(&queue->head, headIndex, ((headIndex + VERSIONONES) & VERSIONBITS)|(nextIndex & INDEXBITS))) {
                        return head;
                    }
                }
            }
        }
    }

	return NULL;
}

pthread_barrier_t b;

void* arrays[ARRAYNUM * ARRAYLEN];
int queues = ARRAYNUM;

int count;
void* routine(void* argv) {
	Node* list[NUMOFMAXPER];
	int num = 0;
	for(;;){
		Node* node = get_one_node();
		if(node == NULL)
			break;	
		int index = __sync_fetch_and_add(&count, 1);
		arrays[index] = node;
		//node->pointer = (void*) index;
		list[num] = node;
		++num; 
		if(num == NUMOFMAXPER)
			break;
	}
	pthread_barrier_wait(&b);
	for(int i = 0; i < num; ++i)
		return_the_node(list[i]);
	return NULL;	
}

int count2;
int noCount;
void* routine2(void* argv) {
    for(;;){
        Node* node = get_one_node();
        if(node == NULL)
           	return NULL;
        //if(arrays[(int) node->pointer] != node) {
//			__sync_fetch_and_add(&noCount, 1);
	//	} 
        __sync_fetch_and_add(&count2, 1);
    }
}

#define 	numOfItems 		1024
int refCount[numOfItems];  
int recordremove;
pthread_barrier_t barrier;
void* recordremoveRoutine(void *argv) {
	struct timeval start;
	int seed = (int) argv;
	srand(seed);
	int key = rand() & (numOfItems - 1);
	for(int i = 0; i < recordremove; ++i) {
  	//		int key = rand() & (numOfItems - 1);
			int refc = refCount[key];
			gettimeofday(&start, NULL);
			int k = 0;
			if((start.tv_usec & 111) >= 0) {
				for(;;) {
				if(refc < ((1 << REFCB) - 1)) { 
					if( __sync_bool_compare_and_swap(refCount + key, refc, refc + 1)) {
						psly_record(key * (ARRAYLEN-1)/* * INDEXBITS*/);
						break;
					}
					refc = refCount[key];		
				} else break;
				}
			}
			key = (key + 1) & (numOfItems - 1);	
	}
	pthread_barrier_wait(&barrier);
//	printf("%d has gone\n", pthread_self());
	int startKey = rand() & (numOfItems - 1);
	int j = 0;
	int p = 0;
	for(int i = startKey; j < numOfItems; ++j) {
		int k = 0;
		for(;;) {
			int ref = refCount[i];
			if(ref <= 0)
				break;
			if(__sync_bool_compare_and_swap(refCount + i, ref, ref - 1)) {
				psly_remove(i * (ARRAYLEN-1));	
			}	
			if((++k) % 10000 == 0) 
				printf("has goes %d times\n", k);	
		}
		i = (++i) & (numOfItems - 1);
	} 	
	return NULL;	
}

int main(int argc, char** argv) {
	if(argc != 3)
		return 1;
	int num = atoi(argv[1]);
	pthread_barrier_init(&barrier, NULL, num);
	recordremove = atoi(argv[2]);	
	float time_use=0;
    struct timeval start;
    struct timeval end;
	gettimeofday(&start, NULL);

	pthread_t pids[NUMOFTHREAD];


	gettimeofday(&start, NULL);


	for(int i = 0; i < ARRAYNUM; ++i){
        Record* record;
        records[i] = record = (Record*) malloc(ARRAYLEN * sizeof(Record));
        for(int j = 0; j < ARRAYLEN - 1; ++j){
            record->self = (i << IDXNUM) | j;
            record->next = j+1;
            record->nextRecord = 0;
            record->pointer = NULL;
            record += 1;
        }
        record->self = (i << IDXNUM) | (ARRAYLEN - 1);
        record->next = NEXTTAILBIT;
        record->nextRecord = 0;
        record->pointer = NULL;
        recordQueues[i].head = 0;
        recordQueues[i].tail = ARRAYLEN - 1;
    }
	
	for(int i = 0; i < ARRAYLEN; ++i) {
		RecordList* list = &map.lists[i];
		Record* head = get_one_record();
		Record* tail = get_one_record();
		head->nextRecord = plusRecord(head->nextRecord);
		head->nextRecord = nxtAddr(head->nextRecord, tail);
		list->head = head;
		list->tail = tail;	
	}
	
	gettimeofday(&start, NULL);

	printf("\n\n\n");
	for(int i = 0; i < num; ++i)
        pthread_create(pids + i, NULL, recordremoveRoutine, (void*) i);
	for(int i = 0; i < num; ++i)
		pthread_join(pids[i], NULL);		
	int errCount = 0;
	int total = num * recordremove;
	int add = 0;
	for(int i = 0; i < numOfItems; ++i) {
//		printf("%d, %d\n", refCount[i], psly_search(i * (ARRAYLEN)));
		if(refCount[i] != psly_search(i * (ARRAYLEN-1))) {
			++errCount;
//			printf("%d refCount: %d, %d\n", i, refCount[i], k = psly_search(i * (ARRAYLEN)));	
		}
	}
	printf("contract: %d\n", contract);
	printf("err numbers: %d\n", errCount);
	printf("total: %d, adds: %d\n", total, add);
	gettimeofday(&end,NULL);
    time_use=(end.tv_sec-start.tv_sec)+(end.tv_usec-start.tv_usec) / 1000000.0;//微秒	
	printf("time_use: %f\n\n", time_use);
	return 0;	
}
