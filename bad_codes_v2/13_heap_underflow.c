#include <stdio.h>
#include <stdlib.h>
int main(void) {
    int *p = malloc(sizeof(int) * 10);
    p[-1] = 42;
    printf("%d\n", p[-1]);
    free(p);
    return 0;
}
