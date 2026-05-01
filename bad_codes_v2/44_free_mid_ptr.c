#include <stdio.h>
#include <stdlib.h>
int main(void) {
    char *p = malloc(10);
    char *q = p + 5;
    free(p);
    free(q);
    return 0;
}
