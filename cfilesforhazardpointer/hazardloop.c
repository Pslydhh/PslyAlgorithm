
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>

#define 	K	2	
int	THRESHOLD = 1;
int MAXTHRES = 1;	
#define		MAXTHREADS	11000


typedef struct NodeType {
	int value;
	struct NodeType* next;	
} NodeType;

void* GlobalHP[K * MAXTHREADS];
int H = 0;

typedef struct ListType{
	void* node;
	struct ListType* next;
} ListType;

typedef struct HPrectype{
	void** HP;
	struct HPrectype* next;
	bool Active;
	int	rcount;
	int count;
	ListType* list;
} HPrecType;


HPrecType* head;
NodeType* Head; 
NodeType* Tail;
static __thread HPrecType* myhprec;

void allocateHPRec() {
	HPrecType* local = head;
	for(;local != NULL; local = local->next){
		//printf("allocate\n");
		if(local->Active)
			continue;
		if(!__sync_bool_compare_and_swap(&local->Active, false, true))
			continue;
		myhprec = local;
		return;		
	}
	
	int oldCount;
	do {
		oldCount = H;
	} while(!__sync_bool_compare_and_swap(&H, oldCount, oldCount + K));
	
	local = (HPrecType*) malloc(sizeof(HPrecType));
	local->Active = true;
	local->next = NULL;
	local->HP = GlobalHP + oldCount;
	for(int i = 0; i < K; ++i)
		local->HP[i] = NULL;
	__asm__ volatile("mfence" : : : "cc", "memory");
	local->rcount = 0;
	local->count = 0;
	local->list = (ListType*) malloc(MAXTHRES * sizeof(ListType));
	for(int i = 0; i < MAXTHRES; ++i) {
		local->list[i].node = NULL;
		local->list[i].next = NULL;
	}	

	HPrecType* oldHead;
	do {
		oldHead = head;
		local->next = oldHead;
	} while(!__sync_bool_compare_and_swap(&head, oldHead, local));
	myhprec = local;
}

void retireHPrec(){
	for(int i = 0; i < K; ++i)
		myhprec->HP[i] = NULL;
	myhprec->Active = false;
}	

bool isInpre(void* node, ListType* list){
    while(list != NULL){
        if(list->node == node)
            return true;
		list = list->next;
    }
    return false;
}

bool isIn(void* node){
 /*
	int local = H;
	for(int i = 0; i < local; ++i){
		__asm__ volatile("lfence" : : : "cc", "memory");
		if(GlobalHP[i] != NULL && node == GlobalHP[i])
			return true;
	}		
	return false; */
	return psly_search(node);
}

void freeList(ListType* list){
    while(list != NULL){
        ListType* next = list->next;
        free(list);
        list = next;
    }
}

int numOfRetire;

void scan(HPrecType* local){
	__sync_fetch_and_add(&numOfRetire, 1);
    ListType* rList = local->list;
	int i = 0;
    while(i <= local->rcount){
        void* node = rList[i].node;
        if(node != NULL && !isIn(node)){
			free(node);
		//	printf("free %p\n", node);
			rList[i].node = NULL;
			local->count--;	
		}
		++i;
    }

}
/*
void helpScan(){
    HPrecType* hprec = head;
    for(;hprec != NULL; hprec = hprec->next){
        if(hprec->Active)
            continue;
        if(!__sync_bool_compare_and_swap(&hprec->Active, false, true))
            continue;
        ListType* hpList = hprec->list;
        while(hpList != NULL){
            ListType* list = (ListType*) malloc(sizeof(ListType));
            list->next = myhprec->list;
            list->node = hpList->node;
            myhprec->list = list;
            myhprec->rcount++;

            hpList = hpList->next;
        }

        freeList(hpList);

        if(myhprec->rcount >= THRESHOLD)
            scan(head);
        hprec->Active = false;
    	__asm__ volatile("mfence" : : : "cc", "memory");
    }
}
*/
void retireNode(void* node){
	int i;
	int count = 0;
	begin: i = 0;	
	ListType* the;
/*	count++;
	if(count > 100000)	
		printf("retireLoop %d", count); */
	for(the = myhprec->list; i < MAXTHRES; ){
		if(the->node == NULL)
			break;
		++i;
		the = myhprec->list + i;
	} 
	if(i == MAXTHRES){
		scan(myhprec);
		goto begin;
	}	
	the->node = node;
	if(i > myhprec->rcount)	
		myhprec->rcount = i;
	myhprec->count++;

	//HPrecType* local = head;
	if(myhprec->count >= THRESHOLD){
		scan(myhprec);
		//helpScan();
	}
}

void enqueue(int value){
	NodeType* node = (NodeType*) malloc(sizeof(NodeType));
	//printf("tid: %ld input addr: %p\n", pthread_self(), node);
	node->value = value;
	node->next = NULL;
	NodeType* t;
	int count = 0;
	for(;;){
		count++;
		if(count > 100000)
			printf("LLLLLLLLLLLOOOOOOOOOo\n");
		t = Tail;
	//	myhprec->HP[0] = t; //most
		psly_record(t);
		if(Tail != t) {
			psly_remove(t);
			continue; 
		}
		NodeType* next = t->next;
		if(Tail != t) {
			psly_remove(t);
			continue;
		}
		if(next != NULL){ 
			__sync_bool_compare_and_swap(&Tail, t, next);
			psly_remove(t);
			continue;
		}
		if(__sync_bool_compare_and_swap(&t->next, NULL, node))
			break;
		psly_remove(t);
	}
	__sync_bool_compare_and_swap(&Tail, t, node);
	//myhprec->HP[0] = NULL;
	psly_remove(t);
}
int flagRet = 0;

int dequeue(){
	int data;
	NodeType* h;
	int count = 0;
	for(;;){
		count++;
		if(count > 100000)
			printf("dequeue loop!!!\n");
		h = Head;
		//myhprec->HP[0] = h;
		psly_record(h);
		if(Head != h) {
			psly_remove(h);
			continue;
		}
		NodeType* t = Tail;
		NodeType* next = h->next;
		psly_remove(h);
		if(next == NULL) {
            return -1000000;
        }
		//myhprec->HP[1] = next;
		psly_record(next);
		if(Head != h) {
			psly_remove(next);
			continue;
		}
		if(h == t){
			__sync_bool_compare_and_swap(&Tail, t, next);
			psly_remove(next);
			continue;
		}
		data = next->value;
		//myhprec->HP[1] = NULL;
		//myhprec->HP[0] = NULL;
		psly_remove(next);	
		if(__sync_bool_compare_and_swap(&Head, h, next))
			break;
	}
	//myhprec->HP[0] = NULL;
	//myhprec->HP[1] = NULL;
	if(flagRet)
		retireNode(h);	
	return data;
}	

int numofdequeue = 0;

void* thread_routine(void* argv){
	allocateHPRec();
	for(int i = 100; i < 100 + (int)argv; ++i)
		enqueue(i);
	int result;
	//printf("enqueue end!!! %d\n", pthread_self());
	while((result = dequeue()) != -1000000){
		//printf("tid: %ld output is: %d times is: %d\n", pthread_self(), result, numofdequeue + 1);
		__sync_fetch_and_add(&numofdequeue, 1);	
	}
//	printf("dequeue end!!!!%d\n", pthread_self());
	scan(myhprec);
	return 0;
}

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
#define HEADIDXNUM	IDEXNUM
#define HEADIDXBIT	IDXBIT
//版本号记录
#define HEADVERSIONNUM	(32 - HEADIDXNUM)
#define HEADVERSIONBIT	((~0)^HEADIDXBIT)
#define HEADVERSIONONE	(1 + HEADIDXBIT)
/* for head */
/* 构造链表时使用 */

//pointer指向被记录的地址(不能被free)
typedef struct Record {
    volatile int next;
    int self;

    volatile long nextRecord;
    volatile void* pointer;
} Record;

typedef struct RecordQueue {
    volatile int head;
    volatile int tail;
} RecordQueue;

void* theFront;
int ftimes;
Record* volatile records[ARRAYNUM];
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
                       	if(head == theFront) {
							++ftimes;
							printf("%p has been get %d\n\n", theFront, ftimes);	 
						}
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
						if(head == theFront) {
							++ftimes;	
                            printf("%p has been get %d\n\n", theFront, ftimes);
						}
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

#define NEXTB          14
#define NEXTBITS       ((NODEONE - 1) ^ ((NODEONE - 1) >> NEXTB))
#define NEXTONE		   (((NEXTBITS - 1) & NEXTBITS) ^ NEXTBITS)
#define NEXTTT         (NEXTBITS | ARRIDXBIT_)

#define NODEB          14
#define NODEBITS       ((REFONE -1 ) ^ ((REFONE - 1) >> NODEB))
#define NODEONE        (((NODEBITS - 1) & NODEBITS) ^ NODEBITS)

#define REFCB          (64 - NODEB - NEXTB - ARRNUM - IDXNUM)
#define REFCBITS       (((long)(-1)) << (64 - REFCB))
#define REFCBITS_	   ((1 << REFCB) - 1)
#define REFONE         (((long)1) << (64 - REFCB))
#define DELETED        0x0000000000000000
#define RECBW          (64 - REFCB)

#define RESTART		   2
#define KEEPPREV       1
#define NONE		   0

#define RECORD		   0x00000004
#define SEARCH         0x00000002
#define REMOVE         0x00000001 

#define LISTNUM		   4096
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

long clrNext(long old) {
	return old /*& ~ARRIDXBIT_*/;
}

long tsfNext(long old, Record* replace) {
	if(replace->self == 0)
		printf("OH MYGOD\n");
	return (old & REFCBITS) | (old & NODEBITS) | ((old + NEXTONE) & NEXTBITS) | (replace->self & ARRIDXBIT_);
}

long plsRefc(long old) {
	return ((old + REFONE) & REFCBITS) | (old & NODEBITS) | (old & NEXTBITS) | (old & ARRIDXBIT_);
}

long newNext(long old, Record* replace) {
	return REFONE | ((old + NODEONE) & NODEBITS) | (old & NEXTBITS) | (replace->self & ARRIDXBIT_);
}

int listNum;
int arg7;
int psly_handle_records(RecordList* list, void* pointer, int flag, int n) {
	long key = (long) pointer;
	Record* head = list->head;
	Record* tail = list->tail;
	Record* my;
	int removed = 0;
	// Traverse the ordered list
	Record* prev;		
	Restart:
	prev = head;
	long volatile prevNext = prev->nextRecord;
	Record* volatile curr = idx_record(prevNext);
	if(curr == theFront)
		printf("here Restart %p\n", theFront);
	long volatile currNext;
	Record* volatile found = NULL;
	long volatile foundNext;
	for(;;){
		
		// keep logical prev, and look at remaining record
		KeepPrev:
		if(prev == curr || head == curr) {
			printf("is equals prev:curr %p==%p head: %p\n", prev, curr, head);
			printf("pointer: %p\n", prev->pointer);
			printf("nextRecord %ld\n", prev->nextRecord);
			printf("is deleted %d\n", (prev->nextRecord & REFCBITS) == DELETED);
			sleep(2);
		}
		asm volatile("mfence" ::: "memory");
		currNext = curr->nextRecord;
		if(tail != curr && (currNext == 77777 || currNext == 0)) {
			printf("%p 's currNext is %ld tail:%p head:%p\n", curr, currNext, tail, head);
			__sync_add_and_fetch(&arg7, 1);
			sleep(2);
		}
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
    		if(curr == theFront)
        		printf("here JUSTNEXTCHANGE %p\n", theFront);

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
					return ((foundNext & REFCBITS) >> RECBW) & REFCBITS_;
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
					if((foundNext & REFCBITS) == DELETED) {
						curr = idx_record(foundNext);
						if(curr == theFront) {
        					printf("here foundDELETED %p\n", theFront);
							sleep(2);
						}	
						// remove the deleted records
						CasRemove:
						if(__sync_bool_compare_and_swap(&prev->nextRecord, prevNext, New = tsfNext(prevNext, curr))) {
							if(New == 77777) {
								printf("CasRemove the nextRecord is %ld !!!!!!!!!!!!!!!!!!!!!!!, %p\n", New, curr);
								sleep(5);
							}
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
								long jjj;
								first->nextRecord = jjj = clrNext(first->nextRecord);
								if(jjj == 77777) {
									printf("CAS2 the nextRecord is %ld !!!!!!!!!!!!!!!!!!!!!!!\n", jjj);
                                	sleep(5);
								}
								return_record(first);
								if(n == 0)
									__sync_sub_and_fetch(&listNum, 1);
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
							long ppp;
							my->nextRecord = ppp = newNext(my->nextRecord, curr);
                            if(ppp == 77777) {
                                printf("MY the nextRecord is %ld!!!!!!!!!!!!!!!!!!!!!!!\n", ppp);
								printf("%ld %ld %ld %d %ld\n", REFONE, NODEBITS, NEXTBITS, ARRIDXBIT_, NODEONE);
                                sleep(300);
                            }
							long kkk;
							//__sync_synchronize();
							//asm volatile("mfence" ::: "memory");
							__asm__ __volatile__("mfence": : :"memory");
							if(__sync_bool_compare_and_swap(&prev->nextRecord, prevNext, kkk = tsfNext(prevNext, my))) {
								if(kkk == 77777) {
									printf("KKK the nextRecord is %ld !!!!!!!!!!!!\n", kkk);
									sleep(5);
								}
								if(n == 0) {
									int g = __sync_add_and_fetch(&listNum, 1);
						//			printf("%d\n", g);
								}	
								return 1;
							}
                            // append our record failed.....
							my->pointer = NULL;
							int mmm;	
							my->nextRecord = mmm = localN;
							if(mmm == 0) {
					/*			printf("APPEND FAILED the nextRecord is 0!!!!!!!!!!!!!!!!!\n");
								sleep(5);*/	
							}
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
							long r;
							if((refNum = ((r = __sync_sub_and_fetch(&found->nextRecord, REFONE)) & REFCBITS)) != DELETED)
								return (refNum >> RECBW) & REFCBITS_;
							if(r == 0)
								printf("%p 's nextRecord is 0\n", found);
							removed = 1;
						} else {
							long refNum; 
							if(__sync_bool_compare_and_swap(&found->nextRecord, foundNext, refNum = plsRefc(foundNext))) {
								if((((refNum & REFCBITS) >> RECBW) & REFCBITS) > 400) {	
									printf("%ld\n", ((refNum & REFCBITS) >> RECBW) & REFCBITS_);
									sleep(1);
								}
								return ((refNum & REFCBITS) >> RECBW) & REFCBITS_; 
							}
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
			Record* t = curr;
			if((currNext & REFCBITS) == DELETED) {
				curr = idx_record(currNext);
				if(curr == theFront) {
        			printf("here CURDELETE: %p currNext: %ld isTail: %d\n", theFront, currNext, t == tail);
				}
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
			if(curr == theFront)
        		printf("here curCONTINUE %p\n", theFront);
		}	 	
	}
}

int psly_record(void* pointer) {
	long key = (long) pointer;
	RecordList* list = &map.lists[key & (LISTNUM-1)];
	//printf("%d\n", key & (LISTNUM - 1));
	return psly_handle_records(list, pointer, RECORD, key & (LISTNUM - 1));	
}

int psly_remove(void* pointer) {
	long key =(long) pointer;
	RecordList* list = &map.lists[key & (LISTNUM-1)];
	return psly_handle_records(list, pointer, REMOVE, key & (LISTNUM -1));
}

int psly_search(void* pointer) {
    long key =(long) pointer;
    RecordList* list = &map.lists[key & (LISTNUM-1)];
	//printf("%d\n", key & INDEXBITS);
	return psly_handle_records(list, pointer, SEARCH, key & (LISTNUM -1));
}



int main(int argc, char** argv){
	for(int i = 0; i < (1 << ARRNUM); ++i){
        Record* record;
        records[i] = record = (Record*) malloc((1 << IDXNUM) * sizeof(Record));
		if(i == 0)
			theFront = record;
		printf("begin: %p\n", record);
        for(int j = 0; j < (1 << IDXNUM) - 1; ++j){
			if(record == 0x7fc158d47010)
				printf("has allocate 0x7fc158d47010\n");
            record->self = (i << IDXNUM) | j;
            record->next = j+1;
            record->nextRecord = 77777;
            record->pointer = NULL;
            record += 1;
        }
        if(record == 0x7fc158d47010)
            printf("has allocate 0x7fc158d47010\n");
		printf("end:   %p\n", record);        
		record->self = (i << IDXNUM) | ((1 << IDXNUM) - 1);
        record->next = NEXTTAILBIT;
        record->nextRecord = 77777;
        record->pointer = NULL;
        recordQueues[i].head = 0;
        recordQueues[i].tail = (1 << IDXNUM) - 1;
    }
	
	for(int i = 0; i < LISTNUM; ++i) {
		RecordList* list = &map.lists[i];
		Record* head = get_record();
		Record* tail = get_record();
		head->nextRecord = newNext(head->nextRecord, tail);
		if(head->nextRecord == 0) {
			printf("the head is nextRecord is 0!!!!!!!!!!!\n");
			sleep(5);	
		}
		list->head = head;
		list->tail = tail;	
	}
	int n = 0;
	float time_use=0;
    struct timeval start;
    struct timeval end;
    gettimeofday(&start,NULL);
	Head = Tail = (NodeType*) malloc(sizeof(NodeType));
	Head->value = -1;
	Head->next = NULL;
	if(argc != 5)
		return 0;
	int nThread = atoi(argv[1]);
	MAXTHRES = 10000;
	int numItem = atoi(argv[2]);
	THRESHOLD = atoi(argv[4]);
	flagRet = atoi(argv[3]);
	printf("\n");
	pthread_t pid[MAXTHREADS];
	for(int i = 0; i < nThread; ++i)
		pthread_create(&pid[i], NULL, thread_routine, numItem);
	for(int i = 0; i < nThread; ++i)
		pthread_join(pid[i], NULL);
	printf("%d dequeues\n\n", numofdequeue);
	printf("%d arg7\n", arg7);
//	printf("%d lisnodes\n", psly_listnum());
//	printf("%d max\n", psly_max());
	gettimeofday(&end,NULL);
    time_use=(end.tv_sec-start.tv_sec)+(end.tv_usec-start.tv_usec) / 1000000.0;//微秒
    printf("time_use is %f \n",time_use);
	printf("NodeType: %d\n", sizeof(NodeType));
	HPrecType* h = head;
	HPrecType* l;
	while(h != NULL) {
		l = h->next;
		scan(h);
		free(h->list);
		free(h);
		h = l;
	}
	free(Head);
	
	return 0;
}
