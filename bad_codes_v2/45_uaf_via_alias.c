#include <stdio.h>
#include <stdlib.h>
int main(void) {
    int *p = malloc(sizeof(int));
    *p = 42;
    int *q = p;
    *q = 99;
    free(p);
    *q = 100;
    return 0;
}
