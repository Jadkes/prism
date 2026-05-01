#include <stdlib.h>
#include <stdio.h>
int main(void) {
    int *p = malloc(sizeof(int));
    *p = 42;
    free(p);
    int *q = malloc(sizeof(int));
    *q = 99;
    printf("dangling: %d\n", *p);
    free(q);
    return 0;
}
