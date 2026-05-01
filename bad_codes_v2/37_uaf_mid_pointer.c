#include <stdio.h>
#include <stdlib.h>
int main(void) {
    int *p = malloc(sizeof(int) * 10);
    int *q = &p[5];
    free(p);
    *q = 42;
    return 0;
}
