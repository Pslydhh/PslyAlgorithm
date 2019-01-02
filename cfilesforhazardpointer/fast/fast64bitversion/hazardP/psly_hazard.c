#include <stdio.h>
#include <stdlib.h>

#define DEFINE_RESOURCE(Record, idxNum, arrNum, type1, nextRecord, type2, pointer) \
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
}

#define INIT_RESOURCE(Record)   \
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
    } 


DEFINE_RESOURCE_LIST(Record, 16, 4, long, nextRecord, void*, pointer)

int main() {
	INIT_RESOURCE(Record);	

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
	printf("%d\n", kk->next);
	printf("%d\n", IDXNUM_);
	return 0;
}
