#include <stdio.h>
#include <stdlib.h>
int main(void) {
    int *p = malloc(sizeof(int) * 10);
    free(p);
    free(p);
    return 0;
}
