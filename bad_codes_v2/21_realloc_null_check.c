#include <stdio.h>
#include <stdlib.h>
int main(void) {
    int *p = NULL;
    p = realloc(p, 100);
    p[0] = 42;
    free(p);
    return 0;
}
