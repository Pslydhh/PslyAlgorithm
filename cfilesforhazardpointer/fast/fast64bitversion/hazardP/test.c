#include <stdio.h>
#include <stdlib.h>

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
int PSLY_##Record##_NEXTIDXNUM	= idxNum;  \
int PSLY_##Record##_NEXTIDXBIT	= ((1 << idxNum) - 1);  \
             \
int PSLY_##Record##_NEXTTAILNUM	= 1;   \
int PSLY_##Record##_NEXTTAILBIT	= (((1 << 1) - 1) << idxNum);   \
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
    int next; \
    int self; \
\
    type1 nextRecord; \
    type2 pointer; \
} Record; \
\
typedef struct Record##Queue { \
    int head; \
    int tail; \
} Record##Queue;  \
Record* psly_##Record##s[arrNum]; \
Record##Queue psly_##Record##_queues[arrNum]; \
int recordTake = 0;  \
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
            int headIndex = queue->head;\
            int indexHead = headIndex & PSLY_##Record##_HEADIDXBIT;\
            Record* head = arr + indexHead;\
            int tailIndex = queue->tail;\
            int indexTail = tailIndex & PSLY_##Record##_TAILIDXBIT;\
            int nextIndex = head->next;\
\
            if(headIndex == queue->head) {\
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
            int headIndex = queue->head;\
            int indexHead = headIndex & PSLY_##Record##_HEADIDXBIT;\
            Record* head = arr + indexHead;\
            int tailIndex = queue->tail;\
            int indexTail = tailIndex & PSLY_##Record##_TAILIDXBIT;\
\
            int nextIndex = head->next;\
\
            if(headIndex == queue->head) {\
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
    record->next |= PSLY_##Record##_NEXTTAILBIT;\
    int self = record->self;\
    int array = (self >> PSLY_##Record##_IDXNUM) & PSLY_##Record##_ARRBITR;\
    Record* arr = psly_##Record##s[array];\
    RecordQueue* queue = psly_##Record##_queues + array;\
    for(;;) {\
        int tailIndex = queue->tail;\
        int indexTail = tailIndex & PSLY_##Record##_TAILIDXBIT;\
        Record* tail = arr + indexTail;\
        int nextIndex = tail->next;\
\
        if(tailIndex == queue->tail){\
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
int  IDXNUM_ =		idxNum;                   \
int  IDXBIT_=		((1 << idxNum) - 1);	\
       \
int  ARRNUM_	=	arrNum;  \
int  ARRBIT_	=	(((1 << arrNum) - 1) << idxNum);  \
int  ARRIDXBIT_=	(arrNum | idxNum);  \
  \
int  REFCB     =     refNum;\
int  REFCBITS  =     (((long)(-1)) << (64 - refNum));\
int  REFONE    =     (((long)1) << (64 - refNum));\
int  DELETED   =     0x0000000000000000;\
int  RECBW     =     (64 - refNum);\
\
int  NODEB     =     ((64 - refNum - arrNum - idxNum) >> 1);\
int  NODEBITS  =     (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)));\
int  NODEONE   =     ((((((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) - 1) & (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) ^ (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))));\
\
int  NEXTB     =     ((64 - refNum - arrNum - idxNum) >> 1);\
int  NEXTBITS  =     ((((((((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) - 1) & (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) ^ (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) - 1) ^ ((((((((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) - 1) & (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) ^ (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)));\
int  NEXTONE    =      (((((((((((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) - 1) & (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) ^ (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) - 1) ^ ((((((((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) - 1) & (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) ^ (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) - 1) & ((((((((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) - 1) & (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) ^ (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) - 1) ^ ((((((((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) - 1) & (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) ^ (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) ^ ((((((((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) - 1) & (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) ^ (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) - 1) ^ ((((((((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) - 1) & (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) ^ (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))));\
int NEXTTT = (((((((((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) - 1) & (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) ^ (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) - 1) ^ ((((((((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) - 1) & (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) ^ (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) | (arrNum | idxNum));\
\
int  RESTART	=	   2;\
int  KEEPPREV  =     1;\
int  NONE		=   0;\
\
int  RECORD	=	   0x00000004;\
int  SEARCH    =     0x00000002;\
int  REMOVE    =     0x00000001;\
\
typedef struct RecordList {\
    Record* head;\
	Record* tail;\
} RecordList;\
\
typedef struct RecordMap {\
    RecordList lists[listNum];\
} RecordMap;\
\
RecordMap map;\
\
long nxtAddrV(long old, Record* replace) {\
	return (old & REFCBITS) | (old & NODEBITS) | ((old + NEXTONE) & NEXTBITS) | ((replace->self & ARRBIT_) | (replace->self & IDXBIT_));\
}\
\
long nxtRecord(long old) {\
	return REFONE | ((old + NODEONE) & NODEBITS) | (old & NEXTBITS) | (old & ARRIDXBIT_);\
}\
\
long unNxtRecord(long old) {\
	return 0 | ((old - NODEONE) & NODEBITS) | (old & NEXTBITS) | (old & ARRIDXBIT_);\
}\
\
long plusRecord(long old) {\
	return ((old + REFONE) & REFCBITS) | (old & NODEBITS) | (old & NEXTBITS) | (old & ARRIDXBIT_);\
}\
\
long nxtAddr(long old, Record* replace) {\
	return (old & ~ARRIDXBIT_) | ((replace->self & ARRBIT_) | (replace->self & IDXBIT_));\
}\
\
long newNext(long old, Record* replace) {\
	return ((old + REFONE) & REFCBITS) | ((old + NODEONE) & NODEBITS) | (old & NEXTBITS) | (replace->self & ARRIDXBIT_);\
}\
\
int psly_handle_records(RecordList* list, void* pointer, int flag) {\
	long key = (long) pointer;\
	Record* head = list->head;\
	Record* tail = list->tail;\
	Record* my;\
	int removed = 0;\
	Record* prev;		\
	Restart:	\
	prev = head;\
	long prevNext = prev->nextRecord;\
	Record* curr = idx_Record(prevNext);\
	long currNext;\
	Record* found = NULL;\
	long foundNext;\
	for(;;){\
		\
		KeepPrev:\
		currNext = curr->nextRecord;\
		void* currPointer = curr->pointer;\
		long New = prev->nextRecord;\
\
		if((prevNext & NODEBITS) != (New & NODEBITS) || (New & REFCBITS) == DELETED)\
			goto Restart;\
		if((prevNext & NEXTTT) != (New & NEXTTT)) {\
           	JUSTNEXTCHANGE: \
			prevNext = New;\
            curr = idx_Record(prevNext);\
			found = NULL;\
            goto KeepPrev;\
        }\
		prevNext = New;\
		long currKey = (long) currPointer;\
		if(curr == tail || currKey > key) {\
			if(found != NULL) {\
				if(flag == SEARCH) \
					return ((foundNext & REFCBITS) >> RECBW) & ((1 << REFCB) - 1) ;\
				for(;;) {\
					New = found->nextRecord;\
					if((foundNext & NODEBITS) != (New & NODEBITS)) {\
						if(flag == REMOVE)\
							return 0;\
						New = prev->nextRecord;\
                        if((prevNext & NODEBITS) != (New & NODEBITS) || (New & REFCBITS) == DELETED)\
                            goto Restart;\
                        goto JUSTNEXTCHANGE;\
					}\
					foundNext = New;\
					if((foundNext & REFCBITS) == DELETED) {\
						curr = idx_Record(foundNext);	\
						CasRemove:\
						if(__sync_bool_compare_and_swap(&prev->nextRecord, prevNext, New = nxtAddrV(prevNext, curr))) {\
							long local = prevNext;\
							prevNext = New;\
							int STEP = NONE;\
							do {	\
								Record* first = idx_Record(local);\
								local = first->nextRecord;\
								first->pointer = NULL;\
								return_Record(first);\
								if(flag == RECORD) {\
									New = prev->nextRecord;\
									if((prevNext & NODEBITS) != (New & NODEBITS) || (New & REFCBITS) == DELETED) \
										STEP = (STEP < RESTART ? RESTART : STEP);\
   			                    	if((prevNext & NEXTTT) != (New & NEXTTT)) \
										STEP = (STEP < KEEPPREV ? KEEPPREV : STEP);\
									prevNext = New;\
								}	\
							} while((local & ARRIDXBIT_) != (curr->self & ARRIDXBIT_));\
							if(flag == REMOVE)\
								return 0;\
							if(STEP == RESTART)\
								goto Restart;\
							if(STEP == KEEPPREV) \
								goto JUSTNEXTCHANGE; \
\
							long localN; \
							CasAppend:\
					        my = get_Record();\
							New = prev->nextRecord;\
                            if((prevNext & NODEBITS) != (New & NODEBITS) || (New & REFCBITS) == DELETED) {\
                               	return_Record(my); \
								goto Restart;\
							}\
                            if((prevNext & NEXTTT) != (New & NEXTTT)) {\
								return_Record(my);\
                                goto JUSTNEXTCHANGE;\
							}\
							localN = my->nextRecord;\
							my->pointer = pointer;\
							my->nextRecord = newNext(my->nextRecord, curr);\
							if(__sync_bool_compare_and_swap(&prev->nextRecord, prevNext, nxtAddrV(prevNext, my))) \
								return 1;\
							my->pointer = NULL;	\
							my->nextRecord = localN;\
							return_Record(my);\
							New = prev->nextRecord;\
							if((prevNext & NODEBITS) != (New & NODEBITS) || (New & REFCBITS) == DELETED)\
            					goto Restart;\
							if((prevNext & NEXTTT) != (New & NEXTTT)) \
								goto JUSTNEXTCHANGE;\
							prevNext = New;\
							goto CasAppend;		\
						}\
						New = prev->nextRecord;\
                        if((prevNext & NODEBITS) != (New & NODEBITS) || (New & REFCBITS) == DELETED)\
                            goto Restart;\
                        if((prevNext & NEXTTT) != (New & NEXTTT)) \
                            goto JUSTNEXTCHANGE;\
						prevNext = New;\
                        goto CasRemove;	 	\
					} else {\
						if(flag == REMOVE) {\
							long refNum_;\
							if((refNum_ = (__sync_sub_and_fetch(&found->nextRecord, REFONE) & REFCBITS)) != DELETED)\
								return (refNum_ >> RECBW) & ((1 << REFCB) -1);\
							removed = 1;\
						} else {\
							long refNum_;   \
							if(__sync_bool_compare_and_swap(&found->nextRecord, foundNext, refNum_ = plusRecord(foundNext))) \
								return ((refNum_ & REFCBITS) >> RECBW) & ((1 << REFCB) -1);       \
						}  	   \
					}\
				}				\
			}\
			if(flag == SEARCH)\
				return 0;\
			if((prevNext & ARRIDXBIT_) != (curr->self & ARRIDXBIT_)) {\
				goto CasRemove;		\
			} else { \
				if(removed)\
					return 0;	\
				goto CasAppend;	\
			}\
		} else {\
			New = curr->nextRecord;\
			if((currNext & NODEBITS) != (New & NODEBITS)) {\
				New = prev->nextRecord;\
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
				if(removed)\
					return 0;\
				found = curr;\
				foundNext = currNext;\
			}\
			curr = idx_Record(currNext);\
		}	 	\
	}\
}\
\
int psly_record(void* pointer) {\
	long key = (long) pointer;\
	RecordList* list = &map.lists[key & (listNum-1)];\
	return psly_handle_records(list, pointer, RECORD);	\
}\
\
int psly_remove(void* pointer) {\
	long key =(long) pointer;\
	RecordList* list = &map.lists[key & (listNum-1)];\
	return psly_handle_records(list, pointer, REMOVE);\
}\
\
int psly_search(void* pointer) {\
    long key =(long) pointer;\
    RecordList* list = &map.lists[key & (listNum-1)];\
	return psly_handle_records(list, pointer, SEARCH);  \
}


#define INIT_RESOURCE(Record, listNum)   \
	for(int i = 0; i < (1 << PSLY_##Record##_ARRNUM); ++i){ \
        Record* record; \
        psly_##Record##s[i] = record = (Record*) malloc((1 << PSLY_##Record##_IDXNUM) * sizeof(Record)); \
        for(int j = 0; j < (1 << PSLY_##Record##_IDXNUM) - 1; ++j){ \
            record->self = (i << PSLY_##Record##_IDXNUM) | j; \
            record->next = j+1; \
            record += 1; \
        }    \
        record->self = (i << PSLY_##Record##_IDXNUM) | ((1 << PSLY_##Record##_IDXNUM) - 1); \
        record->next = PSLY_##Record##_NEXTTAILBIT;  \
        psly_##Record##_queues[i].head = 0; \
        psly_##Record##_queues[i].tail = (1 << PSLY_##Record##_IDXNUM) - 1; \
    } \
	for(int i = 0; i < listNum; ++i) { \
		RecordList* list = &map.lists[i]; \
		Record* head = get_Record();\
		Record* tail = get_Record();\
		head->nextRecord = plusRecord(head->nextRecord); \
		head->nextRecord = nxtAddr(head->nextRecord, tail); \
		list->head = head; \
		list->tail = tail;	\
	}

#define \
DEFINE_RESOURCE_FOR_HAZARD(Record, refNum, idxNum, arrNum, listNum) \
DEFINE_RESOURCE(Record, refNum, idxNum, arrNum, long, nextRecord, void*, pointer, listNum)

DEFINE_RESOURCE_FOR_HAZARD(Record, 10, 16, 4, 512)

int main() {
	INIT_RESOURCE(Record, 512);	

	printf("%d\n", PSLY_Record_IDXNUM);
	printf("%d\n", PSLY_Record_ARRBITR);
	printf("%d\n", PSLY_Record_HEADVERSIONONE);
	Record k;
	Record* kk = get_Record();
	kk->next = 5555;
	k.next = 1;
	k.nextRecord = 5555;
	printf("%d\n", k.next);
	printf("%ld\n", k.nextRecord);
	printf("%d\n", sizeof(RecordQueue));
	printf("%d\n", sizeof(get_Record));
	printf("%d\n", sizeof(return_Record));
	printf("%d\n", sizeof(psly_Records));
	printf("%d\n", sizeof(psly_Record_queues));
	printf("%d\n", sizeof(idx_Record));
	printf("%d\n", kk->next);
	return 0;
}
