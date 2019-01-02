// 该版本是一个有问题的版本, 原因有两点：
// 1: 我们使用的资源队列长度是10bits(ARRAYPERb),当我们采用1000个线程去record操作时很容易，使用完整个队列，意味着刚被释放的Record资源马上被投入使用。
// 2: 该版本中的record操作，我们在刚开始就获取了一个资源，同时设置引用系数1，并且叠加了1个自身的版本号。同时在释放该Record时没有撤销这两个操作。所以造成自身版本号无意义的
//    增加。
// 3: Record自身的版本号NODEB(1),bits过少，造成很容易就迭代完回到了自身(ABA问题).

// 当我们拿1000个线程去record一个地址时,REFCB(3)意味着引用最多为7，所以当那个看到REF为7的线程，准备原子操作给他+1，本来应该会变为0。但是这个瞬间之前假如有其他线程已经这么做了，接着把这个Record记录放回资源队列，过短的资源队列导致其他线程立刻重用了它，接着给了它REF为1,版本号+1，接着放回。该过程执行两遍就会让版本号回到最初的值，并且由于REF没
// 有撤销，那么一开始的原子操作失败后就会以为Record只是改变了引用计数，然后给这个错误的Record的REF+1，从而我们丢失了。
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#include <pthread.h>

#define NUMOFTHREAD		10240	
#define NUMOFMAXPER     100000

#define ARRAYNUMb 2
#define ARRAYNUM  (1 << ARRAYNUMb)
#define ARRAYPERb 10
#define ARRAYBITS (ARRAYNUM - 1) 
#define ARRAYLEN  (1 << ARRAYPERb)
#define UNUSE          ARRAYLEN
#define INDEXBITS      (UNUSE - 1)
#define VERSIONBITS    (~(1 << (ARRAYNUMb + ARRAYPERb) | ((1 << (ARRAYNUMb + ARRAYPERb))-1)))
#define VERSIONONES    (1 << (ARRAYNUMb + 1 +  ARRAYPERb))
#define ARRSHIFT       (ARRAYPERb + 1)
#define ARRBITS        ((ARRAYNUM - 1) << ARRSHIFT) 

#define ARRINDEXBITS   (((ARRAYNUM - 1) << ARRAYPERb) | (ARRAYLEN - 1))
#define RARRBITS	   ((ARRAYNUM - 1) << ARRAYPERb)
#define RIDXBITS	   (ARRAYLEN - 1)
#define INDEXSHIFT     0

#define REFCB		   3
#define REFCBITS	   ((-1) << (32 - REFCB))
#define REFONE         (1 << (32 - REFCB))
#define DELETED        0x00000000
#define RECBW          (32 - REFCB)

#define NODEB		   1
#define NODEBITS	   ((REFONE -1 ) ^ ((REFONE - 1) >> NODEB))
#define NODEONE		   (((NODEBITS - 1) & NODEBITS) ^ NODEBITS)

#define NEXTB          16
#define NEXTBITS       ((NODEONE - 1) ^ ((NODEONE - 1) >> NEXTB))
#define NEXTONE		   (((NEXTBITS - 1) & NEXTBITS) ^ NEXTBITS)

#define NEXTTT         (NEXTBITS | ARRINDEXBITS)

#define RESTART		   2
#define KEEPPREV       1
#define NONE		   0

#define RECORD		   0x00000004
#define SEARCH         0x00000002
#define REMOVE         0x00000001 

//#define RUNTIME		   1
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
    RecordList lists[1024];
} RecordMap;

RecordMap map;
Record* records[ARRAYNUM];
RecordQueue recordQueues[ARRAYNUM];
int recordTake = 0;

Record* get_one_record() {
	for(int i = 0; i < ARRAYNUM; ++i) {
		int array = __sync_fetch_and_add(&recordTake, 1) & ARRAYBITS;
		//printf("%d\n", array);
		RecordQueue* queue = recordQueues + array;
		Record* arr = records[array];	
		for(;;){
			int headIndex = queue->head;
			int indexHead = headIndex & INDEXBITS;
			Record* head = arr + indexHead;
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
        RecordQueue* queue = recordQueues + array;
        Record* arr = records[array];
        for(;;){
            int headIndex = queue->head;
            int indexHead = headIndex & INDEXBITS;
            Record* head = arr + indexHead;
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

void return_the_record(Record* record) {
	record->next |= UNUSE;
	int self = record->self;
	int arrayIndex = (self & RARRBITS) >> ARRAYPERb;
	Record* arr = records[arrayIndex];
	RecordQueue* queue = recordQueues + arrayIndex;
	
	for(;;) {
		int tailIndex = queue->tail;
		int indexTail = tailIndex & INDEXBITS;
		Record* tail = arr + indexTail;
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

Record* takeNext(int nextRecord) {
	return records[(nextRecord & RARRBITS) >> ARRAYPERb] + (nextRecord & RIDXBITS);
}

int nxtAddrV(int old, Record* replace) {
	return (old & REFCBITS) | (old & NODEBITS) | ((old + NEXTONE) & NEXTBITS) | ((replace->self & RARRBITS) | (replace->self & RIDXBITS));
}

int nxtRecord(int old) {
	return REFONE | ((old + NODEONE) & NODEBITS) | (old & NEXTBITS) | (old & ARRINDEXBITS);
}

int clearRecord(int old) {
	return DELETED | ((old - NODEONE) & NODEBITS) | (old & NEXTBITS) | (old & ARRINDEXBITS);
}

int plusRecord(int old) {
	return ((old + REFONE) & REFCBITS) | (old & NODEBITS) | (old & NEXTBITS) | (old & ARRINDEXBITS);
}

int equalRecords(int old, int new) {
	return (old & REFCBITS) == (new & REFCBITS);
}

int changeNext(int old) {
	return 0;
}

int nxtAddr(int old, Record* replace) {
	return (old & ~ARRINDEXBITS) | ((replace->self & RARRBITS) | (replace->self & RIDXBITS));
}

int newNext(int old, Record* replace) {
	return ((old + REFONE) & REFCBITS) | (old & NODEBITS) | (old & NEXTBITS) | (replace->self & ARRINDEXBITS);
}

void removeRecord(RecordList* list, long key, int version) {

}

int psly_handle_records(RecordList* list, void* pointer, int flag) {
	long key = (long) pointer;
	Record* head = list->head;
	Record* tail = list->tail;
	Record* my;
	if(flag == RECORD) {
		my = get_one_record();
		my->nextRecord = nxtRecord(my->nextRecord);
		my->pointer = pointer;
	}	
	// Traverse the ordered list
	Record* prev;		
	Restart:	
	prev = head;
	int prevNext = prev->nextRecord;
	Record* curr = takeNext(prevNext);
	int currNext;
	Record* found = NULL;
	int foundNext;
	for(;;){
		
		// keep logical prev, and look at remaining record
		KeepPrev:
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
					return (((found->nextRecord) & REFCBITS) >> RECBW) & ((1 << REFCB) - 1) ;
				// process the found record
				int num = 0;
				int numind = 0;
				for(;;) {
					//printf("found\n");
					New = found->nextRecord;
					if((foundNext & NODEBITS) != (New & NODEBITS)) {
						if(flag == REMOVE)
							return false;
						//goto Restart;
						New = prev->nextRecord;
                        if((prevNext & NODEBITS) != (New & NODEBITS) || (New & REFCBITS) == DELETED)
                            goto Restart;
                        goto JUSTNEXTCHANGE;
					}
				#ifndef LOOP
					foundNext = New;
					if((foundNext & REFCBITS) == DELETED) {
				#else
					if((New & REFCBITS) == DELETED) {
				#endif 
						// remove the deleted records
						if((++numind) > 100000000)
							printf("%d tims dele\n", numind);
						CasRemove:
						if(__sync_bool_compare_and_swap(&prev->nextRecord, prevNext, New = nxtAddrV(prevNext, curr))) {
							// remove records succes!!!
							// return the records to respository
							int local = prevNext;
							prevNext = New;
							int STEP = NONE;
							do {	
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
								return false;
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
							New = prev->nextRecord;
                            if((prevNext & NODEBITS) != (New & NODEBITS) || (New & REFCBITS) == DELETED) {
								goto Restart;
							}
                            if((prevNext & NEXTTT) != (New & NEXTTT)) {
                                goto JUSTNEXTCHANGE;
							}
							localN = my->nextRecord;
							my->pointer = pointer;
					//		my->nextRecord = REFONE | my->nextRecord;
							my->nextRecord = nxtAddr(my->nextRecord, curr);
							//printf("here\n");
							if(__sync_bool_compare_and_swap(&prev->nextRecord, prevNext, nxtAddrV(prevNext, my))) 
								return true;
                            // append our record failed.....
							my->pointer = NULL;	
							my->nextRecord = localN;
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
							if((__sync_sub_and_fetch(&found->nextRecord, REFONE) & REFCBITS) != DELETED)
								return true;
						} else {
							if((++num) % 100000000 ==0)
								printf("%d loops\n", num);
							/*if(pointer != found->pointer) {
								New = prev->nextRecord;
 			                    if((prevNext & NODEBITS) != (New & NODEBITS) || (New & REFCBITS) == DELETED)
                            		goto Restart;
                        		goto JUSTNEXTCHANGE;
							} */ 
							/*if(!equalRecords(foundNext, (foundNext = New))) {
								New = prev->nextRecord;
                        		if((prevNext & NODEBITS) != (New & NODEBITS) || (New & REFCBITS) == DELETED)
                            		goto Restart;
                        		if((prevNext & NEXTTT) != (New & NEXTTT))
                            		goto JUSTNEXTCHANGE;		
							}	*/
							if(!__sync_bool_compare_and_swap(&found->nextRecord, foundNext, plusRecord(foundNext)))
								continue;
							my->nextRecord = clearRecord(my->nextRecord);
					   //   	my->nextRecord = (~REFCBITS) & my->nextRecord;
						#ifdef BARRIER	
							__asm__ volatile ("mfence" : : : "cc", "memory");	
						#endif
							return_the_record(my);
							return true;
						}	
					}
				}				
			}
			//printf("not found\n");
			// SEARCH & REMOVE
			if(flag == SEARCH || flag == REMOVE)
				return false;
			
			// without this record
			if((prevNext & ARRINDEXBITS) != (curr->self & ARRINDEXBITS)) {
				//printf("to remove\n");
				goto CasRemove;		
			} else {
				//printf("to append\n");
				//my->nextRecord = nxtAddr(my->nextRecord, curr);	
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
			} else {
				found = curr;
				foundNext = currNext;
			}
			curr = takeNext(currNext);
		}	 	
	}
}

void psly_record(void* pointer) {
	long key = (long) pointer;
	RecordList* list = &map.lists[key & 1023];
	psly_handle_records(list, pointer, RECORD);	
}

void psly_remove(void* pointer) {
	long key =(long) pointer;
	RecordList* list = &map.lists[key & 1023];
	psly_handle_records(list, pointer, REMOVE);
}

int psly_search(void* pointer) {
    long key =(long) pointer;
    RecordList* list = &map.lists[key & 1023];
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
int done = 0;
int runTims;
void* searchThe(void* argv) {
	for(int i = 0; i < runTims; ++i)
		psly_record(argv);
	//printf("%d, has done!\n", __sync_add_and_fetch(&done, 1));	
	return 0;
}

int main(int argc, char** argv){
	if(argc != 4)
		return 0;
	int num = atoi(argv[1]);
	int value = atoi(argv[2]);
	runTims = atoi(argv[3]);	
	pthread_barrier_init(&b, NULL, num);
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
            record->self = (i << ARRAYPERb) | j;
            record->next = (i << ARRSHIFT) | (j+1);
            record->nextRecord = 0;
            record->pointer = NULL;
            record += 1;
        }
        record->self = (i << ARRAYPERb) | (ARRAYLEN - 1);
        record->next = UNUSE;
        record->nextRecord = 0;
        record->pointer = NULL;
        recordQueues[i].head = 0;
        recordQueues[i].tail = ARRAYLEN - 1;
    }
	
	for(int i = 0; i < 1024; ++i) {
//		printf("%d\n", i);
		RecordList* list = &map.lists[i];
		Record* head = get_one_record();
		Record* tail = get_one_record();
		head->nextRecord = plusRecord(head->nextRecord);
		head->nextRecord = nxtAddr(head->nextRecord, tail);
		list->head = head;
		list->tail = tail;	
	}

	gettimeofday(&end,NULL);
    time_use=(end.tv_sec-start.tv_sec)+(end.tv_usec-start.tv_usec) / 1000000.0;//微秒
	
	printf("time_use: %f\n\n", time_use);
	
	gettimeofday(&start, NULL);
	int addr = value;

	printf("\n\n\n");
	for(int i = 0; i < num; ++i)
        pthread_create(pids + i, NULL, searchThe, (void*) addr);
	for(int i = 0; i < num; ++i)
		pthread_join(pids[i], NULL);		

	gettimeofday(&end,NULL);
    time_use=(end.tv_sec-start.tv_sec)+(end.tv_usec-start.tv_usec) / 1000000.0;//微秒	
	printf("times: %d\n\n", psly_search((void*) addr));	
	printf("time_use: %f\n\n", time_use);
	return 0;	
}
