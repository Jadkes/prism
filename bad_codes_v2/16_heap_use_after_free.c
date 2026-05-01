#include <stdio.h>
#include <stdlib.h>
int main(void) {
    int *p = malloc(sizeof(int));
    free(p);
    p[0] = 42;
    return 0;
}
