#include <stdio.h>
#include <stdlib.h>
int main(void) {
    char *p = malloc(10);
    free(p);
    char *q = malloc(10);
    free(q);
    free(p);
    return 0;
}
