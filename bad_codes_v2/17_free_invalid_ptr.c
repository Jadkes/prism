#include <stdio.h>
#include <stdlib.h>
int main(void) {
    int *p = (int *)0x1234;
    free(p);
    return 0;
}
