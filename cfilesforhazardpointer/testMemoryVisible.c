#include <stdio.h>
#include <stdlib.h>

#define	N	30000
int* B;

void* read(){
	for(int i = 1; i <= N; ++i){
		while(B[i] == 0){printf("%d\n", B[i]);}
		printf("%d first output!!!\n", B[i]);
	}
	return NULL;
}

void* write(){
	for(int i = 1; i <= N; ++i){
		B[i] = 1;	
	}	
	printf("Done\n");
	return NULL;	
}

int main(){
	B = (int*) malloc((N+1) * sizeof(int));
	pthread_t pid[2];
	
	pthread_create(&pid[0], NULL, read, NULL);
	sleep(1);
	pthread_create(&pid[1], NULL, write, NULL);
	
	pthread_join(pid[0], NULL);
	pthread_join(pid[1], NULL);

	return 0;	
}
