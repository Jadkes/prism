#include <stdio.h>
#include <stdlib.h>
int main(void) {
    int n = 1000000000;
    int *p = malloc(sizeof(int) * n);
    if (p) p[0] = 42;
    free(p);
    return 0;
}
