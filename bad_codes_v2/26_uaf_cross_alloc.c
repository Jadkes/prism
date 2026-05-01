#include <stdio.h>
#include <stdlib.h>
int main(void) {
    int *p = malloc(sizeof(int));
    *p = 1;
    free(p);
    int *q = malloc(sizeof(int));
    *q = *p;
    free(q);
    return 0;
}
