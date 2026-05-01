#include <stdio.h>
#include <stdlib.h>
int main(void) {
    int *p = malloc(sizeof(int) * 100);
    for (int i = 0; i <= 100; i++)
        p[i] = i;
    free(p);
    return 0;
}
