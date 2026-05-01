#include <stdio.h>
#include <stdlib.h>
int main(void) {
    int *p = malloc(sizeof(int));
    *p = 42;
    free(p);
    int *q = malloc(sizeof(int));
    *q = *p + 1;
    printf("%d\n", *q);
    free(q);
    return 0;
}
