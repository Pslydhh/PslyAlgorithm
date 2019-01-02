#include <stdio.h>

int main(){
	void* p = (void*) -1;
	printf("%p\n", ((long)p) << 3);
}
