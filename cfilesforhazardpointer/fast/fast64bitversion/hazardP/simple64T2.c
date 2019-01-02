#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>

#define CACHE_LINE_SIZE 64
#define CACHE_ALIGNED __attribute__((aligned(CACHE_LINE_SIZE)))
#define DOUBLE_CACHE_ALIGNED __attribute__((aligned(2 * CACHE_LINE_SIZE)))

struct timeval start;

#define DEFINE_RESOURCE(Record, refNum, idxNum, arrNum, type1, nextRecord, type2, pointer, listNum) \
int PSLY_##Record##_IDXNUM = idxNum; \
int PSLY_##Record##_IDXBIT = ((1 << idxNum) - 1); \
 \
int PSLY_##Record##_ARRNUM = arrNum;             \
int PSLY_##Record##_ARRAYNUM = (1 << arrNum);     \
int PSLY_##Record##_ARRAYBITS = ((1 << arrNum) -1);  \
int PSLY_##Record##_ARRBIT = (((1 << arrNum) - 1) << idxNum);  \
int PSLY_##Record##_ARRBITR = ((1 << arrNum) - 1);  \
\
int PSLY_##Record##_ARRIDXBIT = ((((1 << arrNum) - 1) << idxNum) | ((1 << idxNum) - 1)); \
\
int PSLY_##Record##_NEXTIDXNUM  = idxNum;  \
int PSLY_##Record##_NEXTIDXBIT  = ((1 << idxNum) - 1);  \
             \
int PSLY_##Record##_NEXTTAILNUM = 1;   \
int PSLY_##Record##_NEXTTAILBIT = (((1 << 1) - 1) << idxNum);   \
   \
int PSLY_##Record##_NEXTVERSIONNUM = (32 - 1 - idxNum);      \
int PSLY_##Record##_NEXTVERSIONBIT = ((~0)^((((1 << 1) - 1) << idxNum) | ((1 << idxNum) - 1)));  \
int PSLY_##Record##_NEXTVERSIONONE = (1 + ((((1 << 1) - 1) << idxNum) | ((1 << idxNum) - 1))); \
  \
  \
int PSLY_##Record##_TAILIDXNUM = idxNum;  \
int PSLY_##Record##_TAILIDXBIT = ((1 << idxNum) - 1);  \
  \
int PSLY_##Record##_TAILVERSIONNUM = (32 - idxNum);  \
int PSLY_##Record##_TAILVERSIONBIT = ((~0) ^ ((1 << idxNum) - 1));   \
int PSLY_##Record##_TAILVERSIONONE = (1 + ((1 << idxNum) - 1));  \
   \
   \
int PSLY_##Record##_HEADIDXNUM = idxNum;  \
int PSLY_##Record##_HEADIDXBIT = ((1 << idxNum) - 1);   \
   \
int PSLY_##Record##_HEADVERSIONNUM = (32 - idxNum);   \
int PSLY_##Record##_HEADVERSIONBIT = ((~0) ^ ((1 << idxNum) - 1));  \
int PSLY_##Record##_HEADVERSIONONE = (1 + ((1 << idxNum) - 1));  \
typedef struct Record { \
    int volatile next ; \
    int self ; \
    type1 volatile nextRecord ; \
    type2 volatile pointer ; \
} Record ; \
\
typedef struct Record##Queue { \
    int volatile head ; \
    int volatile tail ; \
} Record##Queue ;  \
static Record* volatile psly_##Record##s[1 << arrNum]; \
static Record##Queue volatile psly_##Record##_queues[1 << arrNum]; \
static int volatile recordTake = 0;  \
  \
Record* idx_##Record(int index) {   \
    return psly_##Record##s[(index & PSLY_##Record##_ARRBIT) >> PSLY_##Record##_IDXNUM] + (index & PSLY_##Record##_IDXBIT);   \
} \
   \
      \
Record* get_##Record() {             \
    for(int i = 0; i < PSLY_##Record##_ARRAYNUM; ++i) {\
        int array = __sync_fetch_and_add(&recordTake, 1) & PSLY_##Record##_ARRAYBITS;\
        RecordQueue* queue = psly_##Record##_queues + array;\
    Record* arr = psly_##Record##s[array];\
        for(;;){\
            int headIndex = (queue->head);\
            int indexHead = headIndex & PSLY_##Record##_HEADIDXBIT;\
            Record* head = arr + indexHead;\
            int tailIndex = (queue->tail);\
            int indexTail = tailIndex & PSLY_##Record##_TAILIDXBIT;\
            int nextIndex = (head->next);\
\
            if(headIndex == (queue->head)) {\
                if(indexHead == indexTail){\
                    if((nextIndex & PSLY_##Record##_NEXTTAILBIT) == PSLY_##Record##_NEXTTAILBIT)\
                        break;\
                    __sync_bool_compare_and_swap(&queue->tail, tailIndex, (((tailIndex & PSLY_##Record##_TAILVERSIONBIT) + PSLY_##Record##_TAILVERSIONONE ) & PSLY_##Record##_TAILVERSIONBIT)|(nextIndex & PSLY_##Record##_TAILIDXBIT));    \
                } else {\
                    if(__sync_bool_compare_and_swap(&queue->head, headIndex, (((headIndex & PSLY_##Record##_HEADVERSIONBIT) + PSLY_##Record##_HEADVERSIONONE) & PSLY_##Record##_HEADVERSIONBIT)|(nextIndex & PSLY_##Record##_HEADIDXBIT))) {\
                        return head;\
                    }\
                }\
            }\
        }\
    }\
  \
    for(int i = 0; i < PSLY_##Record##_ARRAYNUM; ++i) {\
        int array = i;\
        RecordQueue* queue = psly_##Record##_queues + array;\
        Record* arr = psly_##Record##s[array];\
        for(;;){\
            int headIndex = (queue->head);\
            int indexHead = headIndex & PSLY_##Record##_HEADIDXBIT;\
            Record* head = arr + indexHead;\
            int tailIndex = (queue->tail);\
            int indexTail = tailIndex & PSLY_##Record##_TAILIDXBIT;\
\
            int nextIndex = (head->next);\
\
            if(headIndex == (queue->head)) {\
                if(indexHead == indexTail){\
                    if((nextIndex & PSLY_##Record##_NEXTTAILBIT) == PSLY_##Record##_NEXTTAILBIT)\
                        break;\
                    __sync_bool_compare_and_swap(&queue->tail, tailIndex, (((tailIndex & PSLY_##Record##_TAILVERSIONBIT) + PSLY_##Record##_TAILVERSIONONE ) & PSLY_##Record##_TAILVERSIONBIT)|(nextIndex & PSLY_##Record##_TAILIDXBIT));    \
                } else {\
                    if(__sync_bool_compare_and_swap(&queue->head, headIndex, (((headIndex & PSLY_##Record##_HEADVERSIONBIT) + PSLY_##Record##_HEADVERSIONONE) & PSLY_##Record##_HEADVERSIONBIT)|(nextIndex & PSLY_##Record##_HEADIDXBIT))) {\
                        return head;\
                    }\
                }\
            }\
\
        }\
    }\
  return NULL;\
}\
  \
void return_##Record(Record* record) {\
  long local = (record->next);\
  local |= PSLY_##Record##_NEXTTAILBIT;\
    /*record->next |= PSLY_##Record##_NEXTTAILBIT;*/\
  record->next = local;\
    int self = record->self;\
    int array = (self >> PSLY_##Record##_IDXNUM) & PSLY_##Record##_ARRBITR;\
    Record* arr = psly_##Record##s[array];\
    RecordQueue* queue = psly_##Record##_queues + array;\
    for(;;) {\
        int tailIndex = (queue->tail);\
        int indexTail = tailIndex & PSLY_##Record##_TAILIDXBIT;\
        Record* tail = arr + indexTail;\
        int nextIndex = (tail->next);\
\
        if(tailIndex == (queue->tail)){\
            if((nextIndex & PSLY_##Record##_NEXTTAILBIT) == PSLY_##Record##_NEXTTAILBIT) {\
                if(__sync_bool_compare_and_swap(&tail->next, nextIndex, (((nextIndex & PSLY_##Record##_NEXTVERSIONBIT) + PSLY_##Record##_NEXTVERSIONONE) & PSLY_##Record##_NEXTVERSIONBIT)|(self & PSLY_##Record##_NEXTIDXBIT))){\
                    __sync_bool_compare_and_swap(&queue->tail, tailIndex, (((tailIndex & PSLY_##Record##_TAILVERSIONBIT) + PSLY_##Record##_TAILVERSIONONE) & PSLY_##Record##_TAILVERSIONBIT)|(self & PSLY_##Record##_TAILIDXBIT));\
                    return;\
                }\
            } else {\
                __sync_bool_compare_and_swap(&queue->tail, tailIndex, (((tailIndex & PSLY_##Record##_TAILVERSIONBIT) + PSLY_##Record##_TAILVERSIONONE) & PSLY_##Record##_TAILVERSIONBIT)|(nextIndex & PSLY_##Record##_TAILIDXBIT));\
            }\
        }\
    }\
}  \
int  IDXNUM_ =    idxNum;                   \
long  IDXBIT_=    ((1 << idxNum) - 1);  \
       \
int  ARRNUM_  = arrNum;  \
long  ARRBIT_ = (((1 << arrNum) - 1) << idxNum);  \
long  ARRIDXBIT_= (((1 << idxNum) - 1) | (((1 << arrNum) - 1) << idxNum));  \
  \
int  REFCB     =     refNum;\
int  REFCBITS_ = ((1 << refNum) - 1);\
long  REFCBITS  =     (((long)(-1)) << (64 - refNum));\
long  REFONE    =     (((long)1) << (64 - refNum));\
long  DELETED   =     0x0000000000000000;\
int  RECBW     =     (64 - refNum);\
\
int  NODEB     =     ((64 - refNum - arrNum - idxNum) >> 1);\
long  NODEBITS  =     (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)));\
long  NODEONE   =     ((((((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) - 1) & (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) ^ (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))));\
\
int  NEXTB     =     ((64 - refNum - arrNum - idxNum) >> 1);\
long  NEXTBITS  =     ((((((((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) - 1) & (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) ^ (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) - 1) ^ ((((((((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) - 1) & (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) ^ (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)));\
long  NEXTONE   =     (((((((((((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) - 1) & (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) ^ (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) - 1) ^ ((((((((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) - 1) & (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) ^ (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) - 1) & ((((((((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) - 1) & (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) ^ (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) - 1) ^ ((((((((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) - 1) & (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) ^ (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) ^ ((((((((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) - 1) & (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) ^ (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) - 1) ^ ((((((((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) - 1) & (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) ^ (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))));\
long  NEXTTT    =   (((((((((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) - 1) & (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) ^ (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) - 1) ^ ((((((((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) - 1) & (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) ^ (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) | ((((1 << arrNum) - 1) << idxNum)|((1 << idxNum) - 1)));\
\
int  RESTART  =    2;\
int  KEEPPREV  =     1;\
int  NONE   =   0;\
\
int  RECORD =    0x00000004;\
int  SEARCH    =     0x00000002;\
int  REMOVE    =     0x00000001;\
\
typedef struct RecordList {\
    Record* volatile head ;\
  Record* volatile tail ;\
} RecordList ;\
\
typedef struct RecordMap {\
    volatile RecordList* lists[listNum] ;\
} RecordMap ;\
\
static volatile RecordMap map;\
\
long nxtAddrV(long old, Record* replace) {\
  return (old & REFCBITS) | (old & NODEBITS) | ((old + NEXTONE) & NEXTBITS) | (replace->self & ARRIDXBIT_);\
}\
\
long plusRecord(long old) {\
  return ((old + REFONE) & REFCBITS) | (old & NODEBITS) | (old & NEXTBITS) | (old & ARRIDXBIT_);\
}\
\
long newNext(long old, Record* replace) {\
  return REFONE | ((old + NODEONE) & NODEBITS) | (old & NEXTBITS) | (replace->self & ARRIDXBIT_);\
}\
\
int psly_handle_records(RecordList* list, void* pointer, int flag) { \
  long key = (long) pointer;\
  volatile Record* head = list->head;\
  volatile Record* tail = list->tail;\
  volatile Record* my = NULL;\
  long localN;\
  int removed = 0;\
  volatile Record* prev;    \
  volatile long prevBefore;\
  Restart:  \
  prev = head;\
  volatile long prevNext = (prev->nextRecord);\
  volatile Record* curr = idx_Record(prevNext);\
  volatile long currNext;\
  volatile Record* found = NULL;\
  volatile long foundNext;\
  for(;;){\
    KeepPrev:\
    currNext = curr->nextRecord;\
    void* currPointer = (curr->pointer);\
    long New = (prev->nextRecord);\
    if((prevNext & NODEBITS) != (New & NODEBITS) || (New & REFCBITS) == DELETED)\
      goto Restart;\
    if((prevNext & NEXTTT) != (New & NEXTTT)) {\
      JUSTNEXTCHANGE: \
      prevNext = New;\
      curr = idx_Record(prevNext);\
      goto KeepPrev;\
    }\
	prevNext = New;\
\
    long currKey = (long) currPointer;\
    if(curr == tail || currKey > key) {\
		if(flag == SEARCH) \
			return 0;\
	  	if(flag == REMOVE && (prevNext & ARRIDXBIT_) == (curr->self & ARRIDXBIT_))\
			return 0;\
		Record* append;\
		CasAppend:\
	    if(flag == RECORD) {\
			if(my == NULL) {\
				my = get_Record();\
				localN = (my->nextRecord);\
				my->pointer = pointer;\
			}\
            my->nextRecord = newNext(localN, curr);\
			append = my;\
		} else {\
			append = curr;\
		}\
		if((prevBefore = __sync_val_compare_and_swap(&prev->nextRecord, prevNext, New = nxtAddrV(prevNext, append))) == prevNext ) {\
		  long local = prevNext;\
		  while((local & ARRIDXBIT_) != (curr->self & ARRIDXBIT_)){\
			Record* first = idx_Record(local);\
			first->pointer = NULL;\
			local = (first->nextRecord);\
			return_Record(first);\
		  }\
		  if(flag == RECORD)\
			return 1;\
		  else\
			return 0;\
		}\
		New = prevBefore;\
		if((prevNext & NODEBITS) != (New & NODEBITS) || (New & REFCBITS) == DELETED)\
		  goto Restart;\
		if((prevNext & NEXTTT) != (New & NEXTTT)) \
		  goto JUSTNEXTCHANGE;\
		prevNext = New;\
		goto CasAppend; \
    } else {\
      New = (curr->nextRecord);\
      if((currNext & NODEBITS) != (New & NODEBITS)) {\
        New = (prev->nextRecord);\
        if((prevNext & NODEBITS) != (New & NODEBITS) || (New & REFCBITS) == DELETED)\
          goto Restart;\
        goto JUSTNEXTCHANGE;\
      }\
      currNext = New;\
      if((currNext & REFCBITS) == DELETED) {\
        curr = idx_Record(currNext);\
        continue;\
      }\
      if(currKey != key) {\
        prev = curr;\
        prevNext = currNext;\
      } else {\
		if(flag == SEARCH) \
          return ((currNext & REFCBITS) >> RECBW) & REFCBITS_;\
        if(removed)\
          return 0;\
	  \
        found = curr;\
        foundNext = currNext;\
		if(flag == REMOVE) {\
		  long refNum_;\
		  if(((refNum_ = __sync_sub_and_fetch(&found->nextRecord, REFONE)) & REFCBITS) != DELETED)\
			return (refNum_ >> RECBW) & REFCBITS_;\
		  removed = 1;\
		  currNext = refNum_;\
		} else {\
			for(;;) {\
				long refNum_;   \
				if((prevBefore = __sync_val_compare_and_swap(&found->nextRecord, foundNext, refNum_ = plusRecord(foundNext))) == foundNext) {\
					if(my != NULL) {\
						my->nextRecord = localN;\
						return_Record(my);\
					}\
					return (refNum_ >> RECBW) & REFCBITS_;       \
				}\
				New = prevBefore;\
				if((foundNext & NODEBITS) != (New & NODEBITS)) {\
					New = (prev->nextRecord);\
					if((prevNext & NODEBITS) != (New & NODEBITS) || (New & REFCBITS) == DELETED)\
						goto Restart;\
					goto JUSTNEXTCHANGE;\
				}\
				if((New & REFCBITS) == DELETED) {\
					currNext = New;\
					break;\
				}\
				foundNext = New;\
			}\
        }\
      }\
      curr = idx_Record(currNext);\
    }   \
  }\
}\
\
int psly_record(void* pointer) {\
  long key = (long) pointer;\
  RecordList* list = map.lists[key & (listNum-1)];\
  return psly_handle_records(list, pointer, RECORD);  \
}\
\
int psly_remove(void* pointer) {\
  long key =(long) pointer;\
  RecordList* list = map.lists[key & (listNum-1)];\
  return psly_handle_records(list, pointer, REMOVE);\
}\
\
int psly_search(void* pointer) {\
  long key =(long) pointer;\
  RecordList* list = map.lists[key & (listNum-1)];\
  return psly_handle_records(list, pointer, SEARCH);  \
}


#define INIT_RESOURCE(Record, listNum)   \
  for(int i = 0; i < (1 << PSLY_##Record##_ARRNUM); ++i){ \
    Record* record; \
    void * ptr;\
    int ret = posix_memalign(&ptr, 4096, (1 << PSLY_##Record##_IDXNUM) * sizeof(Record));\
    psly_##Record##s[i] = record = ptr; \
    memset(record, 0, (1 << PSLY_##Record##_IDXNUM) * sizeof(Record)); \
    for(int j = 0; j < (1 << PSLY_##Record##_IDXNUM) - 1; ++j){ \
      record->self = (i << PSLY_##Record##_IDXNUM) | j; \
      record->next = j+1; \
	  record->pointer = NULL;\
      record->nextRecord = (j % 2 == 1)?39415:39419;\
	  /*printf("%ld\n", record->nextRecord);*/\
      record += 1; \
    }    \
    record->self = (i << PSLY_##Record##_IDXNUM) | ((1 << PSLY_##Record##_IDXNUM) - 1); \
    record->next = PSLY_##Record##_NEXTTAILBIT;  \
	record->pointer = NULL;\
    record->nextRecord = 39415;\
    psly_##Record##_queues[i].head = 0; \
    psly_##Record##_queues[i].tail = (1 << PSLY_##Record##_IDXNUM) - 1; \
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

#define UNINIT_RESOURCE(Record, listNum)  \
    for(int i = 0; i < (1 << PSLY_##Record##_ARRNUM); ++i){ \
        free(psly_##Record##s[i]); \
    } \
	for(int i = 0; i < listNum; ++i) {\
		free(map.lists[i]);\
	}

#define \
DEFINE_RESOURCE_FOR_HAZARD(Record, refNum, idxNum, arrNum, listNum) \
DEFINE_RESOURCE(Record, refNum, idxNum, arrNum, long, nextRecord, void*, pointer, listNum)

#define LISTNUM 512
DEFINE_RESOURCE_FOR_HAZARD(Record, 16, 16, 4, LISTNUM) 


pthread_barrier_t b;


int count;

int count2;
int noCount;
int flagStart;
#define   numOfItems      1024
#define   listN		  (LISTNUM -1)
int refCount[numOfItems];  
int recordremove;
pthread_barrier_t barrier;
pthread_mutex_t   mutex;
int totalOperation;
pthread_barrier_t barrier;
void* recordremoveRoutine(void *argv) {
  int seed = (int) argv;
  srand(seed);
  int key = rand() & (numOfItems - 1);
  for(int i = 0; i < recordremove; ++i) {
	  if(key == 0)
		key = 1;
      int refc = refCount[key];
      for(;;) {
        if(refc < REFCBITS_) {
          if( __sync_bool_compare_and_swap(refCount + key, refc, refc + 1)) {
            psly_record(key * (listN));
            __sync_fetch_and_add(&totalOperation, 1);
            psly_remove(key * (listN));
            __sync_fetch_and_sub(refCount + key, 1);
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
}

#define   NUMOFTHREAD   10240
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
  gettimeofday(&start, NULL);

  printf("\n\n\n");
  int errCount = 0;
  pthread_mutex_lock(&mutex);
  gettimeofday(&start, NULL);
  pthread_setconcurrency(sysconf(_SC_NPROCESSORS_ONLN));
  for(int i = 0; i < num; ++i)
        pthread_create(pids + i, NULL, recordremoveRoutine, (void*) i);
  pthread_barrier_wait(&barrier);
  for(int i = 0; i < numOfItems; ++i) {
	if(i == 0)
		continue;
    int re = psly_search(i * (listN));
    if( i <= 1000) {
    }
    if(refCount[i] != re) 
            ++errCount;
  }
  pthread_mutex_unlock(&mutex);
  for(int i = 0; i < num; ++i)
    pthread_join(pids[i], NULL);    
  int total = num * recordremove;
  int add = 0;
  for(int i = 0; i < numOfItems; ++i) {
    int re; 
	if(i == 0)
		continue;
    if(refCount[i] != (re = psly_search(i * (listN)))) {
      ++errCount;
    }
  } 
  printf("err numbers: %d\n", errCount);
  printf("total: %d, adds: %d\n", total, add);
  printf("actual op: %d\n", totalOperation);
  gettimeofday(&end,NULL);
      time_use=(end.tv_sec-start.tv_sec)+(end.tv_usec-start.tv_usec) / 1000000.0;//微秒
  printf("total_use: %f\n\n", time_use);
  int k = 0;
  for(int i = 0; i < LISTNUM; ++i) {
    Record* head = map.lists[i]->head;
    Record* tail = map.lists[i]->tail;
    while(head != tail) {
      ++k;
      head = idx_Record(head->nextRecord);
    }   
  }  
  UNINIT_RESOURCE(Record, LISTNUM);
  printf("%d, %d, %d\n", LISTNUM, k, LISTNUM == k);
  return 0; 
}
