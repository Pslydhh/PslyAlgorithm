#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
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
	int local = H;
	for(int i = 0; i < local; ++i){
		__asm__ volatile("lfence" : : : "cc", "memory");
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
    ListType* rList = myhprec->list;
	int i = 0;
    while(i <= myhprec->rcount){
        void* node = rList[i].node;
        if(node != NULL && !isIn(node)){
			free(node);
		//	printf("free %p\n", node);
			rList[i].node = NULL;
			myhprec->count--;	
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
	begin: i = 0;	
	ListType* the;
	for(the = myhprec->list; i < MAXTHRES; ){
		if(the->node == NULL)
			break;
		++i;
		the = myhprec->list + i;
	} 
	if(i == MAXTHRES){
		scan(NULL);
		goto begin;
	}	
	the->node = node;
	if(i > myhprec->rcount)	
		myhprec->rcount = i;
	myhprec->count++;

	HPrecType* local = head;
	if(myhprec->count >= THRESHOLD){
		scan(local);
		//helpScan();
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
		if(Tail != t) {
			myhprec->HP[0] = NULL;
			continue;
		}
		NodeType* next = t->next;
		if(Tail != t) {
			myhprec->HP[0] = NULL;
			continue;
		}
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
		myhprec->HP[1] = NULL;
		myhprec->HP[0] = NULL;
		
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
//	if(myhprec == NULL)
//		printf("IS null\n");return;
	float time_use=0;
    struct timeval start;
    struct timeval end;
//	NodeType* kkk = (void*) malloc(sizeof(NodeType));
    gettimeofday(&start,NULL);
	Head = Tail = (NodeType*) malloc(sizeof(NodeType));
	Head->value = -1;
	Head->next = NULL;
	if(argc != 5)
		return 0;
	int nThread = atoi(argv[1]);
	MAXTHRES = nThread * 2 + 1;
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
	gettimeofday(&end,NULL);
    time_use=(end.tv_sec-start.tv_sec)+(end.tv_usec-start.tv_usec) / 1000000.0;//微秒
    printf("time_use is %f \n",time_use);
	printf("NodeType: %d\n", sizeof(NodeType));
	HPrecType* h = head;
	HPrecType* l;
//	scan(NULL);
	while(h != NULL) {
		free(h->list);
		l = h->next;
		free(h);
		h = l;	
	}
//	sleep(10);	
	return 0;
}
