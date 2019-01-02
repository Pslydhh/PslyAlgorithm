#include <stdio.h>
#define ACQUIRE(p) ({ \
  __typeof__(*(p)) __ret = *p; \
  __asm__("":::"memory"); \
  __ret; \
})

#define RELEASE(p, v) do {\
  __asm__("":::"memory"); \
  *p = v; \
} while (0)

int main() {
    int k;
	k = 0;
	asm ("":::"memory");
    printf("%d\n",  k);
    return 0;
}
