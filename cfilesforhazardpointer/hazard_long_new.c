#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

int PSLY_Record_IDXNUM = 16; 
int PSLY_Record_IDXBIT = ((1 << 16) - 1); 
 
int PSLY_Record_ARRNUM = 4;             
int PSLY_Record_ARRAYNUM = (1 << 4);     
int PSLY_Record_ARRAYBITS = ((1 << 4) -1);  
int PSLY_Record_ARRBIT = (((1 << 4) - 1) << 16);  
int PSLY_Record_ARRBITR = ((1 << 4) - 1);  

int PSLY_Record_ARRIDXBIT = ((((1 << 4) - 1) << 16) | ((1 << 16) - 1)); 

int PSLY_Record_NEXTIDXNUM  = 16;  
int PSLY_Record_NEXTIDXBIT  = ((1 << 16) - 1);  
             
int PSLY_Record_NEXTTAILNUM = 1;   
int PSLY_Record_NEXTTAILBIT = (((1 << 1) - 1) << 16);   
   
int PSLY_Record_NEXTVERSIONNUM = (32 - 1 - 16);      
int PSLY_Record_NEXTVERSIONBIT = ((~0)^((((1 << 1) - 1) << 16) | ((1 << 16) - 1)));  
int PSLY_Record_NEXTVERSIONONE = (1 + ((((1 << 1) - 1) << 16) | ((1 << 16) - 1))); 
  
  
int PSLY_Record_TAILIDXNUM = 16;  
int PSLY_Record_TAILIDXBIT = ((1 << 16) - 1);  
  
int PSLY_Record_TAILVERSIONNUM = (32 - 16);  
int PSLY_Record_TAILVERSIONBIT = ((~0) ^ ((1 << 16) - 1));   
int PSLY_Record_TAILVERSIONONE = (1 + ((1 << 16) - 1));  
   
   
int PSLY_Record_HEADIDXNUM = 16;  
int PSLY_Record_HEADIDXBIT = ((1 << 16) - 1);   
   
int PSLY_Record_HEADVERSIONNUM = (32 - 16);   
int PSLY_Record_HEADVERSIONBIT = ((~0) ^ ((1 << 16) - 1));  
int PSLY_Record_HEADVERSIONONE = (1 + ((1 << 16) - 1));  

typedef struct Record { 
    int volatile next __attribute__((aligned(128))); 
    int self ; 
	int refcount;
	bool delete;
    long volatile nextRecord __attribute__((aligned(128))); 
    void* volatile pointer ; 
} Record __attribute__((aligned(128))); 

typedef struct RecordQueue { 
    int volatile head ; 
    int volatile tail ; 
} RecordQueue ;  
static Record* volatile psly_Records[1 << 4]; 
static RecordQueue volatile psly_Record_queues[1 << 4]; 
static int volatile recordTake = 0;  
  
Record* idx_Record(int index) {   
    return psly_Records[(index & PSLY_Record_ARRBIT) >> PSLY_Record_IDXNUM] + (index & PSLY_Record_IDXBIT);   
} 
   
      
Record* get_Record() {             
    for(int i = 0; i < PSLY_Record_ARRAYNUM; ++i) {
        int array = __sync_fetch_and_add(&recordTake, 1) & PSLY_Record_ARRAYBITS;
        RecordQueue* queue = psly_Record_queues + array;
    	Record* arr = psly_Records[array];
        for(;;){
            int headIndex = (queue->head);
            int indexHead = headIndex & PSLY_Record_HEADIDXBIT;
            Record* head = arr + indexHead;
            int tailIndex = (queue->tail);
            int indexTail = tailIndex & PSLY_Record_TAILIDXBIT;
            int nextIndex = (head->next);

            if(headIndex == (queue->head)) {
                if(indexHead == indexTail){
                    if((nextIndex & PSLY_Record_NEXTTAILBIT) == PSLY_Record_NEXTTAILBIT)
                        break;
                    __sync_bool_compare_and_swap(&queue->tail, tailIndex, (((tailIndex & PSLY_Record_TAILVERSIONBIT) + PSLY_Record_TAILVERSIONONE ) & PSLY_Record_TAILVERSIONBIT)|(nextIndex & PSLY_Record_TAILIDXBIT));    
                } else {
                    if(__sync_bool_compare_and_swap(&queue->head, headIndex, (((headIndex & PSLY_Record_HEADVERSIONBIT) + PSLY_Record_HEADVERSIONONE) & PSLY_Record_HEADVERSIONBIT)|(nextIndex & PSLY_Record_HEADIDXBIT))) {
                        return head;
                    }
                }
            }
        }
    }
  
    for(int i = 0; i < PSLY_Record_ARRAYNUM; ++i) {
        int array = i;
        RecordQueue* queue = psly_Record_queues + array;
        Record* arr = psly_Records[array];
        for(;;){
            int headIndex = (queue->head);
            int indexHead = headIndex & PSLY_Record_HEADIDXBIT;
            Record* head = arr + indexHead;
            int tailIndex = (queue->tail);
            int indexTail = tailIndex & PSLY_Record_TAILIDXBIT;

            int nextIndex = (head->next);

            if(headIndex == (queue->head)) {
                if(indexHead == indexTail){
                    if((nextIndex & PSLY_Record_NEXTTAILBIT) == PSLY_Record_NEXTTAILBIT)
                        break;
                    __sync_bool_compare_and_swap(&queue->tail, tailIndex, (((tailIndex & PSLY_Record_TAILVERSIONBIT) + PSLY_Record_TAILVERSIONONE ) & PSLY_Record_TAILVERSIONBIT)|(nextIndex & PSLY_Record_TAILIDXBIT));    
                } else {
                    if(__sync_bool_compare_and_swap(&queue->head, headIndex, (((headIndex & PSLY_Record_HEADVERSIONBIT) + PSLY_Record_HEADVERSIONONE) & PSLY_Record_HEADVERSIONBIT)|(nextIndex & PSLY_Record_HEADIDXBIT))) {
                        return head;
                    }
                }
            }

        }
    }
  return NULL;
}
  
void return_Record(Record* record) {
  long local = (record->next);
  local |= PSLY_Record_NEXTTAILBIT;
  record->next = local;
    int self = record->self;
    int array = (self >> PSLY_Record_IDXNUM) & PSLY_Record_ARRBITR;
    Record* arr = psly_Records[array];
    RecordQueue* queue = psly_Record_queues + array;
    for(;;) {
        int tailIndex = (queue->tail);
        int indexTail = tailIndex & PSLY_Record_TAILIDXBIT;
        Record* tail = arr + indexTail;
        int nextIndex = (tail->next);

        if(tailIndex == (queue->tail)){
            if((nextIndex & PSLY_Record_NEXTTAILBIT) == PSLY_Record_NEXTTAILBIT) {
                if(__sync_bool_compare_and_swap(&tail->next, nextIndex, (((nextIndex & PSLY_Record_NEXTVERSIONBIT) + PSLY_Record_NEXTVERSIONONE) & PSLY_Record_NEXTVERSIONBIT)|(self & PSLY_Record_NEXTIDXBIT))){
                    __sync_bool_compare_and_swap(&queue->tail, tailIndex, (((tailIndex & PSLY_Record_TAILVERSIONBIT) + PSLY_Record_TAILVERSIONONE) & PSLY_Record_TAILVERSIONBIT)|(self & PSLY_Record_TAILIDXBIT));
                    return;
                }
            } else {
                __sync_bool_compare_and_swap(&queue->tail, tailIndex, (((tailIndex & PSLY_Record_TAILVERSIONBIT) + PSLY_Record_TAILVERSIONONE) & PSLY_Record_TAILVERSIONBIT)|(nextIndex & PSLY_Record_TAILIDXBIT));
            }
        }
    }
}  
int  IDXNUM_ =    16;                   
long  IDXBIT_=    ((1 << 16) - 1);  
       
int  ARRNUM_  = 4;  
long  ARRBIT_ = (((1 << 4) - 1) << 16);  
long  ARRIDXBIT_= (((1 << 16) - 1) | (((1 << 4) - 1) << 16));  
  
int  REFCBITS_ = ((1 << 16) - 1);
long  REFCBITS  =     (((long)(-1)) << (64 - 16));
long  REFONE    =     (((long)1) << (64 - 16));
long  DELETED   =     1 << 63;
int  RECBW     =     (64 - 16);

long  NODEBITS  =     ((((long)(-1)) << 42) & ((((long)1) << 63) - 1));
long  NODEONE   =     1 << 42;

long  NEXTBITS  =    ((((long) (-1)) << 20) & ((((long)1) << 42) - 1));
long  NEXTONE = 1 << 20; 
long  NEXTTT = ((((long)(-1)) << 42) & ((((long)1) << 63) - 1)) | ((((long) (-1)) << 20) & ((((long)1) << 42) - 1));

int  RESTART  =    2;
int  KEEPPREV  =     1;
int  NONE   =   0;

int  RECORD =    0x00000004;
int  SEARCH    =     0x00000002;
int  CLEAR    =     0x00000001;

typedef struct RecordList {
    Record* volatile head ;
  Record* volatile tail ;
} RecordList ;

typedef struct RecordMap {
    volatile RecordList* lists[131072] ;
} RecordMap ;

static volatile RecordMap map;

long nxtAddrV(long old, Record* replace) {
  return (old & NODEBITS) | ((old + NEXTONE) & NEXTBITS) | (replace->self & ARRIDXBIT_);
}

long newNext(long old, Record* replace) {
  return ((old + NODEONE) & NODEBITS) | (old & NEXTBITS) | (replace->self & ARRIDXBIT_);
}

typedef struct Prevs {
	Record* r __attribute__((aligned(128)));
	long rNext ;
} Prevs __attribute__((aligned(128)));
#define MAXPREV 1024
#define STEPBIT 0
#define STEPS   (1 << STEPBIT)
#define STEPS_  (STEPS - 1)

int psly_handle_records(RecordList* list, void* pointer, int flag) { 
  static __thread Prevs prevs_[MAXPREV];
  static __thread bool flag_ = false;
  if(!flag_) {
  for(int i = 0; i < MAXPREV; ++i) {
	prevs_[i].r = NULL;
	prevs_[i].rNext = 0;
  }
  flag_ = true;
  }
  static __thread int steps; 
  steps = 0;
  long key = (long) pointer;
  Record* head = list->head;
  Record* tail = list->tail;
  steps = 0; 
  Record* my = NULL;
  long localN;
  int removed = 0;
  Record* prev = head;
  long prevNext = (prev->nextRecord);
  Record* curr = idx_Record(prevNext);
  for(;;) {
	long currNext;
    KeepPrev:
    currNext = curr->nextRecord;
    void* currPointer = (curr->pointer);
    long New = (prev->nextRecord);
    if((prevNext & NODEBITS) != (New & NODEBITS) || (New & DELETED) == DELETED) {
	  --steps;
	  for(;;) {
		int bucket = steps >> STEPBIT;
		bucket = bucket < MAXPREV ? bucket: (MAXPREV - 1);
		Prevs* prevs = &prevs_[bucket];
		prev = prevs->r;
		prevNext = prev->nextRecord;
	 	long prevNextKeep = prevs->rNext;
	  	if((prevNextKeep & NODEBITS) != (prevNext & NODEBITS) || (prevNext & DELETED) == DELETED) {
		  steps -= STEPS;	
		} else {
		  prevs->rNext = prevNext;
		  curr = idx_Record(prevNext);
		  break;
		}
	  }
	  steps = steps & (~STEPS_); 
	  continue;
	}
    if((prevNext & NEXTTT) != (New & NEXTTT)) {
      prevNext = New;
      curr = idx_Record(prevNext);
      continue;
    }
	prevNext = New;
    long currKey = (long) currPointer;
    if(curr == tail || currKey > key) {
	  	if(flag == CLEAR && (prevNext & ARRIDXBIT_) == (curr->self & ARRIDXBIT_))
			return NULL;
		Record* append;
	    if(flag == RECORD) {
			if(my == NULL) {
				my = get_Record();
				localN = (my->nextRecord);
				my->pointer = pointer;
				__sync_fetch_and_add(&my->refcount, 1);
				my->delete = false;
			}
            my->nextRecord = newNext(localN, curr);
			append = my;
		} else {
			append = curr;
		}
		for(;;) {
		long prevBefore;
		Record* newRecord;
		if((prevBefore = __sync_val_compare_and_swap(&prev->nextRecord, prevNext, newRecord = nxtAddrV(prevNext, append))) == prevNext ) {
		  long local = prevNext;
		  while((local & ARRIDXBIT_) != (curr->self & ARRIDXBIT_)){
			Record* first = idx_Record(local);
			first->pointer = NULL;
			local = (first->nextRecord);
			return_Record(first);
		  }
		  if(flag == RECORD)
			return newRecord;
		  else
			return NULL;
		}
		New = prevBefore;
		if((prevNext & NODEBITS) != (New & NODEBITS) || (New & DELETED) == DELETED) {
      --steps;
      for(;;) {
        int bucket = steps >> STEPBIT;
        bucket = bucket < MAXPREV ? bucket: (MAXPREV - 1);
        Prevs* prevs = &prevs_[bucket];
        prev = prevs->r;
        prevNext = prev->nextRecord;
        long prevNextKeep = prevs->rNext;
        if((prevNextKeep & NODEBITS) != (prevNext & NODEBITS) || (prevNext & DELETED) == DELETED) {
          steps -= STEPS;
        } else {
          prevs->rNext = prevNext;
          curr = idx_Record(prevNext);
          break;
        }
      }
      steps = steps & (~STEPS_);

      	  goto KeepPrev;
		}
		if((prevNext & NEXTTT) != (New & NEXTTT)) {
          prevNext = New;
          curr = idx_Record(prevNext);
      	  goto KeepPrev;
		}
		prevNext = New;
		}
    } else {
	  if(flag == SEARCH && currKey == key) {
          if(my != NULL) {
              my->nextRecord = localN;
              __sync_fetch_and_sub(&my->refcount, 1);
              return_Record(my);
          }
		  Record* found = curr;
          long foundNext = currNext; 
		  if(found->delete)
              return NULL;
          New = found->nextRecord;
          if((foundNext & NODEBITS) != (New & NODEBITS)) {
              return NULL;
          }
          if((New & DELETED) == DELETED) {
              return NULL;
          }
          __sync_fetch_and_add(&found->refcount, 1);
          if(found->delete) {
              __sync_fetch_and_sub(&found->refcount, 1);
              return NULL;
          }
          New = found->nextRecord;
          if((foundNext & NODEBITS) != (New & NODEBITS)) {
              __sync_fetch_and_sub(&found->refcount, 1);
              return NULL;
          }
          if((New & DELETED) == DELETED) {
              __sync_fetch_and_sub(&found->refcount, 1);
              return NULL;
          }
          return found;
      }
      New = (curr->nextRecord);
      if((currNext & NODEBITS) != (New & NODEBITS)) {
        New = (prev->nextRecord);
        if((prevNext & NODEBITS) != (New & NODEBITS) || (New & DELETED) == DELETED) {
      --steps;
      for(;;) {
        int bucket = steps >> STEPBIT;
        bucket = bucket < MAXPREV ? bucket: (MAXPREV - 1);
        Prevs* prevs = &prevs_[bucket];
        prev = prevs->r;
        prevNext = prev->nextRecord;
        long prevNextKeep = prevs->rNext;
        if((prevNextKeep & NODEBITS) != (prevNext & NODEBITS) || (prevNext & DELETED) == DELETED) {
          steps -= STEPS;
        } else {
          prevs->rNext = prevNext;
          curr = idx_Record(prevNext);
          break;
        }
      }
      steps = steps & (~STEPS_);

          continue;
		}
        prevNext = New;
        curr = idx_Record(prevNext);
        continue;
      }
      currNext = New;
      if((currNext & DELETED) == DELETED) {
        curr = idx_Record(currNext);
        continue;
      }
      if(currKey != key) {
		int bucket;
		if((steps & STEPS_) == 0 && (bucket = (steps >> STEPBIT)) < MAXPREV) {
			Prevs* step = &prevs_[bucket];
			step->r = prev;
			step->rNext = prevNext;
		}
		++steps; 
        prev = curr;
        prevNext = currNext;
		curr = idx_Record(currNext);
      } else {
		return NULL;
	  }
    }   
  }
}

int psly_record(void* pointer) {
  long key = ((long) pointer) >> 4 ;
  RecordList* list = map.lists[key & 131071];
  return psly_handle_records(list, pointer, RECORD);  
}

int psly_remove(void* pointer) {
  long key =((long) pointer) >> 4 ;
  RecordList* list = map.lists[key & 131071];
  return psly_handle_records(list, pointer, CLEAR);
}

int psly_search(void* pointer) {
  long key =((long) pointer) >> 4 ;
  RecordList* list = map.lists[key & 131071];
  return psly_handle_records(list, pointer, SEARCH);  
}

#define INIT_RESOURCE(listNum)   \
  for(int i = 0; i < (1 << PSLY_Record_ARRNUM); ++i){ \
    Record* record; \
    void * ptr;\
    int ret = posix_memalign(&ptr, 4096, (1 << PSLY_Record_IDXNUM) * sizeof(Record));\
    psly_Records[i] = record = ptr; \
    memset(record, 0, (1 << PSLY_Record_IDXNUM) * sizeof(Record)); \
    for(int j = 0; j < (1 << PSLY_Record_IDXNUM) - 1; ++j){ \
      record->self = (i << PSLY_Record_IDXNUM) | j; \
      record->next = j+1; \
	  record->pointer = NULL;\
      record->nextRecord = (j % 2 == 1)?39415:39419;\
	  /*printf("%ld\n", record->nextRecord);*/\
      record += 1; \
    }    \
    record->self = (i << PSLY_Record_IDXNUM) | ((1 << PSLY_Record_IDXNUM) - 1); \
    record->next = PSLY_Record_NEXTTAILBIT;  \
	record->pointer = NULL;\
    record->nextRecord = 39415;\
    psly_Record_queues[i].head = 0; \
    psly_Record_queues[i].tail = (1 << PSLY_Record_IDXNUM) - 1; \
  } \
  for(int i = 0; i < listNum; ++i) { \
    void* ptr;\
    int ret = posix_memalign(&ptr, 4096, sizeof(RecordList));\
    Record* head = get_Record();\
    Record* tail = get_Record();\
    head->nextRecord = newNext(head->nextRecord, tail); \
	map.lists[i] = ptr;\
    map.lists[i]->head = head; \
    map.lists[i]->tail = tail; \
  }

#define UNINIT_RESOURCE(listNum)  \
    for(int i = 0; i < (1 << PSLY_Record_ARRNUM); ++i){ \
        free(psly_Records[i]); \
    } \
	for(int i = 0; i < listNum; ++i) {\
		free(map.lists[i]);\
	}

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
	int times;
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


volatile HPrecType* head;
volatile NodeType* Head; 
volatile NodeType* Tail;
static __thread HPrecType* myhprec;

void allocateHPRec() {
	HPrecType* local = head;
	for(;local != NULL; local = local->next){
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
	if(local == NULL) {printf("NULL!!!!\n");exit(1);}
	local->Active = true;
	local->next = NULL;
	local->HP = GlobalHP + oldCount;
	for(int i = 0; i < K; ++i)
		local->HP[i] = NULL;
	__asm__ volatile("mfence" : : : "cc", "memory");
	local->rcount = 0;
	local->count = 0;
	local->list = (ListType*) malloc(MAXTHRES * sizeof(ListType));
	if(local->list == NULL) {printf("NULL!\n"); exit(1);}
	for(int i = 0; i < MAXTHRES; ++i) {
		local->list[i].node = NULL;
		local->list[i].next = NULL;
		local->list[i].times = 10;
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

bool isIn(ListType* node){
	Record* record = (Record*) node->node;
	if(record->refcount == 0) 
		node->times--;
	if(node->times == 0) {
		long nextRecord = record->nextRecord;
		long nextBefore;
		void* pointer = record->pointer;
		while(nextRecord != (nextBefore =  __sync_val_compare_and_swap(&record->nextRecord, nextRecord, nextRecord | DELETED)))	
			nextRecord = nextBefore;
		psly_remove(pointer);
		return false;
	}
	return true;	
}

int numOfRetire;
void scan(HPrecType* local){
	__sync_fetch_and_add(&numOfRetire, 1);
    ListType* rList = local->list;
	int i = 0;
    while(i <= local->rcount){
        void* pointer = ((Record*)rList[i].node)->pointer;
        if(pointer != NULL && !isIn(&rList[i])){
			free(pointer);
			rList[i].node = NULL;
			local->count--;	
		}
		++i;
    }
}

void retireNode(void* node, Record* record){
	__sync_lock_test_and_set(&record->delete, true);
	int i;
	int count = 0;
	begin: i = 0;	
	ListType* the;
	count++;
	if(count > 100000)	
		printf("retireLoop %d", count); 
	for(the = myhprec->list; i < MAXTHRES; ){
		if(the->node == NULL)
			break;
		++i;
		the = myhprec->list + i;
	} 
	if(i == MAXTHRES){printf("%ld\n", pthread_self());
		scan(myhprec);
		goto begin;
	}	
	the->node = record;
    the->times = 10;
	if(i > myhprec->rcount)	
		myhprec->rcount = i;
	myhprec->count++;

	if(myhprec->count >= THRESHOLD){
		scan(myhprec);
	}
}

void enqueue(int value){
	NodeType* node;
	posix_memalign(&node, 64, sizeof(NodeType));
	memset(node, 0, sizeof(NodeType));
	node->value = value;
	node->next = NULL;
	NodeType* t;
	for(;;){
		t = Tail;
		Record* record = psly_record(t);
		if(record == NULL)
			continue;
		if(Tail != t) {
			__sync_fetch_and_sub(&record->refcount, 1);
			continue; 
		}
		NodeType* next = t->next;
		if(Tail != t) {
			__sync_fetch_and_sub(&record->refcount, 1);
			continue;
		}
		if(next != NULL){ 
			__sync_fetch_and_sub(&record->refcount, 1);
			__sync_bool_compare_and_swap(&Tail, t, next);
			continue;
		}
		if(__sync_bool_compare_and_swap(&t->next, NULL, node)) {
			__sync_fetch_and_sub(&record->refcount, 1);
			break;
		}
		__sync_fetch_and_sub(&record->refcount, 1);
	}
	__sync_bool_compare_and_swap(&Tail, t, node);
}
int flagRet = 0;

int dequeue(){
	int data;
	NodeType* h;
	Record* record1;
	for(;;){
		h = Head;
		record1 = psly_record(h);
		if(record1 == NULL)
			continue;
		if(Head != h) {
			__sync_fetch_and_sub(&record1->refcount, 1);
			continue;
		}
		NodeType* t = Tail;
		NodeType* next = h->next;
		Record* record2 = psly_record(next);
		if(record2 == NULL)
			continue;
		//myhprec->HP[1] = next;
		if(Head != h) {
			__sync_fetch_and_sub(&record1->refcount, 1);
			__sync_fetch_and_sub(&record2->refcount, 1);
			continue;
		}
		if(next == NULL) {
			__sync_fetch_and_sub(&record1->refcount, 1);
			__sync_fetch_and_sub(&record2->refcount, 1);
			return -1000000;	
		}
		if(h == t){
			__sync_fetch_and_sub(&record1->refcount, 1);
			__sync_fetch_and_sub(&record2->refcount, 1);
			__sync_bool_compare_and_swap(&Tail, t, next);
			continue;
		}
		data = next->value;
		//myhprec->HP[1] = NULL;
		//myhprec->HP[0] = NULL;
		if(__sync_bool_compare_and_swap(&Head, h, next)) {
			__sync_fetch_and_sub(&record1->refcount, 1);
			__sync_fetch_and_sub(&record2->refcount, 1);
			break;
		}
		__sync_fetch_and_sub(&record1->refcount, 1);
	    __sync_fetch_and_sub(&record2->refcount, 1);
	}
	//myhprec->HP[0] = NULL;
	//myhprec->HP[1] = NULL;
	if(flagRet)
		retireNode(h, record1);	
	return data;
}	

long numofdequeue;
long numoferr;
void* thread_routine(void* argv){
	allocateHPRec();
	for(int i = 100; i < 100 + (int)argv; ++i)
		enqueue(3781);
	int result;
	//printf("enqueue end!!! %d\n", pthread_self());
	while((result = dequeue()) != -1000000){
		//printf("tid: %ld output is: %d times is: %d\n", pthread_self(), result, numofdequeue + 1);
		__sync_fetch_and_add(&numofdequeue, 1);
		if(result != 3781) 
			__sync_fetch_and_add(&numoferr, 1);	
	}
	scan(myhprec);
	return 0;
}

int main(int argc, char** argv){
	int n = 0;
	INIT_RESOURCE(131072); for(int k = 0;k < 100 ;++k){
	printf("%d times\n", k);
	float time_use=0;
    struct timeval start;
    struct timeval end;
    gettimeofday(&start,NULL);
	Head = Tail = (NodeType*) malloc(sizeof(NodeType));
	if(Head == NULL) {printf("AAAAAAAAAAAA\n"); exit(1);}
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
	printf("%ld dequeues\n\n", numofdequeue);
	gettimeofday(&end,NULL);
    time_use=(end.tv_sec-start.tv_sec)+(end.tv_usec-start.tv_usec) / 1000000.0;//微秒
    printf("time_use is %f, err num is %ld \n", time_use, numoferr);
	printf("NodeType: %d\n", sizeof(NodeType));
	HPrecType* h = head;head = NULL;H = 0;
	HPrecType* l;
	printf("begin father collect garbage\n");
	while(h != NULL) {
		l = h->next;
		scan(h);
		free(h->list);
		free(h);
		h = l;
	}
	free(Head);
	  int kk = 0;
	  for(int i = 0; i < 131072; ++i) {
        Record* head = idx_Record(map.lists[i]->head->nextRecord);
        Record* tail = map.lists[i]->tail;
        while(head != tail) {
		  printf("%p\n", head->pointer);
          ++kk;
          head = idx_Record(head->nextRecord);
        }
  	  }
	  printf("the rest: %d\n", kk);
    }
	UNINIT_RESOURCE(131072);
	return 0;
}

