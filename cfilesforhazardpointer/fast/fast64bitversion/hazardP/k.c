#include<stdio.h>
#define ACQUIRE(p) ({ \
  __typeof__(*(p)) __ret = *p; \
  __asm__("":::"memory"); \
  __ret; \
})

int main() {
	int i = 2341;
	printf("%d\n", ACQUIRE(&i));
	
	return 0;
}
