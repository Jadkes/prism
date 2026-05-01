#include <stdio.h>
#include <stdlib.h>
int main(void) {
    int *p = malloc(sizeof(int) * 5);
    int *q = p + 10;
    *q = 42;
    free(p);
    return 0;
}
