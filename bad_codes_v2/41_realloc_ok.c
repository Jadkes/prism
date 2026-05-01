#include <stdio.h>
#include <stdlib.h>
int main(void) {
    int *p = malloc(sizeof(int));
    *p = 42;
    int *q = realloc(p, sizeof(int) * 2);
    if (!q) { free(p); return 1; }
    q[1] = 99;
    free(q);
    return 0;
}
