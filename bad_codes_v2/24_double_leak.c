#include <stdio.h>
#include <stdlib.h>
int main(void) {
    int *p = malloc(sizeof(int));
    *p = 42;
    /* leak: no free */
    p = malloc(sizeof(int));
    *p = 99;
    free(p);
    return 0;
}
