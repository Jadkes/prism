#include <stdio.h>
#include <stdlib.h>
int main(void) {
    int *p = malloc(sizeof(int) * 3);
    p[0] = 1; p[1] = 2; p[2] = 3;
    int *q = p;
    free(p);
    q[0] = 10;
    q[1] = 20;
    q[2] = 30;
    return 0;
}
