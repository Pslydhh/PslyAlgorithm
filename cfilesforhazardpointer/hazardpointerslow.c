#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>

#define 	K	2	
#define 	THRESHOLD	1	
#define		MAXTHREADS	1000


typedef struct NodeType {
	int value;
	struct NodeType* next;	
} NodeType;

NodeType* GlobalHP[K * MAXTHREADS];
int H = 0;

typedef struct ListType{
	NodeType* node;
	struct ListType* next;
} ListType;

typedef struct HPrectype{
	NodeType* HP[K];
	struct HPrectype* next;
	bool Active;
	int	rcount;
	ListType* list;
} HPrecType;


HPrecType* head;
NodeType* Head; 
NodeType* Tail;
__thread HPrecType* myhprec;

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
//	local->HP = GlobalHP + oldCount;
	for(int i = 0; i < K; ++i)
		local->HP[i] = NULL;
	local->list = NULL;
	local->rcount = 0;
	
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

bool isInpre(NodeType* node, ListType* list){
    while(list != NULL){
        if(list->node == node)
            return true;
		list = list->next;
    }
    return false;
}

bool isIn(NodeType* node){
	int local = H;
	for(int i = 0; i < local; ++i){
		if(GlobalHP[i] != NULL && node == GlobalHP[i])
			return true;
	}		
	return false;
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
    ListType* plist = NULL;
    HPrecType* hprec = local;
    while(hprec != NULL){
        if(hprec != myhprec) {
            for(int i = 0; i < K; ++i){
				__asm__ volatile("mfence" : : : "cc", "memory");
                NodeType* hptr = hprec->HP[i];
                if(hptr != NULL){
                    ListType* pListNode = (ListType*) malloc(sizeof(ListType));
                    pListNode->next = plist;
                    pListNode->node = hptr;
                    plist = pListNode;
                }
            }
        }
        hprec = hprec->next;
    }
    ListType* rList = myhprec->list;
    ListType* prev = NULL;
    while(rList != NULL){
        NodeType* node = (NodeType*) rList->node;
        if(!isInpre(node, plist)){
            if(prev != NULL){
                prev->next = rList->next;
				free(rList);
				rList = prev->next;
                free(node);
            } else {
				myhprec->list = rList->next;
				free(rList);
				rList = myhprec->list;
				free(node);
			}
		} else {
			prev = rList;
			rList = rList->next;
		}
    }

    freeList(plist);
}

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

void retireNode(NodeType* node){
	ListType* list = (ListType*) malloc(sizeof(ListType));
	list->next = myhprec->list;
	list->node = node;
	myhprec->list = list;	
	myhprec->rcount++;
	__asm__ volatile("mfence" : : : "cc", "memory");

	HPrecType* local = head;
	if(myhprec->rcount >= THRESHOLD){
		scan(local);
		helpScan();
	}
}

void enqueue(int value){
	NodeType* node = (NodeType*) malloc(sizeof(NodeType));
	//printf("tid: %ld input addr: %p\n", pthread_self(), node);
	node->value = value;
	node->next = NULL;
	NodeType* t;
	for(;;){
		t = Tail;
		myhprec->HP[0] = t; //most
		__asm__ volatile("mfence" : : : "cc", "memory");
		if(Tail != t)
			continue;
		NodeType* next = t->next;
		if(Tail != t)
			continue;
		if(next != NULL){
			__sync_bool_compare_and_swap(&Tail, t, next);
			continue;
		}
		if(__sync_bool_compare_and_swap(&t->next, NULL, node))
			break;
	}
	__sync_bool_compare_and_swap(&Tail, t, node);
	myhprec->HP[0] = NULL;
	__asm__ volatile("mfence" : : : "cc", "memory");
}
int flagRet = 0;

int dequeue(){
	int data;
	NodeType* h;
	for(;;){
		h = Head;
		myhprec->HP[0] = h;
		__asm__ volatile("mfence" : : : "cc", "memory");
		if(Head != h)
			continue;
		NodeType* t = Tail;
		NodeType* next = h->next;
		myhprec->HP[1] = next;
		__asm__ volatile("mfence" : : : "cc", "memory");
		if(Head != h)
			continue;
		if(next == NULL)
			return -1000000;
		if(h == t){
			__sync_bool_compare_and_swap(&Tail, t, next);
			continue;
		}
		data = next->value;
		if(__sync_bool_compare_and_swap(&Head, h, next))
			break;
	}
	myhprec->HP[0] = NULL;
	myhprec->HP[1] = NULL;
	__asm__ volatile("mfence" : : : "cc", "memory");
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
	while((result = dequeue()) != -1000000){
		//printf("tid: %ld output is: %d times is: %d\n", pthread_self(), result, numofdequeue + 1);
		__sync_fetch_and_add(&numofdequeue, 1);	
	}
	return 0;
}

int main(int argc, char** argv){
//	void* kkk = malloc(10);
//	free(kkk);
//	return 0;
//	free(34242324);
//	printf("NodeType size: %ld int: %ld NodeType*: %ld\n\n\n", sizeof(NodeType), sizeof(int), sizeof(NodeType*));
//	printf("listType size: %ld node: %ld listType*: %ld\n\n\n", sizeof(ListType), sizeof(NodeType*), sizeof(ListType*));
//	return 0;
	float time_use=0;
    struct timeval start;
    struct timeval end;
    gettimeofday(&start,NULL);
	Head = Tail = (NodeType*) malloc(sizeof(NodeType));
	Head->value = -1;
	Head->next = NULL;
	if(argc != 4)
		return 0;
	int nThread = atoi(argv[1]);
	int numItem = atoi(argv[2]);
	flagRet = atoi(argv[3]);
	printf("\n");
	pthread_t pid[10000];
	for(int i = 0; i < nThread; ++i)
		pthread_create(&pid[i], NULL, thread_routine, numItem);

	for(int i = 0; i < nThread; ++i)
		pthread_join(pid[i], NULL);
	printf("%d dequeues\n\n", numofdequeue);
	gettimeofday(&end,NULL);
    time_use=(end.tv_sec-start.tv_sec)+(end.tv_usec-start.tv_usec) / 1000000.0;//微秒
    printf("time_use is %f \n",time_use);
	
	return 0;
}
