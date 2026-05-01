#include <stdio.h>
#include <stdlib.h>
int main(void) {
    int *p = malloc(sizeof(int) * 10);
    for (int i = 0; i < 10; i++) p[i] = i;
    int *q = realloc(p, sizeof(int) * 5);
    if (!q) { free(p); return 1; }
    printf("%d\n", q[8]);
    free(q);
    return 0;
}
