#include <stdio.h>
#include <stdlib.h>
int main(void) {
    int *p = malloc(sizeof(int) * 3);
    p[0] = 1; p[1] = 2; p[2] = 3;
    int sum = 0;
    for (int i = 0; i < 5; i++)
        sum += p[i];
    printf("%d\n", sum);
    free(p);
    return 0;
}
