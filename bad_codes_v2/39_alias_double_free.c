#include <stdio.h>
#include <stdlib.h>
int main(void) {
    int *p = malloc(sizeof(int));
    *p = 1;
    int *q = p;
    free(p);
    free(q);
    return 0;
}
