#include <stdio.h>
#include <stdlib.h>
int main(void) {
    int *p = malloc(sizeof(int));
    *p = 42;
    int *q = realloc(p, 0);
    if (q) *q = 99;
    free(q);
    return 0;
}
