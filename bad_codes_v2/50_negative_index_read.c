#include <stdio.h>
#include <stdlib.h>
int main(void) {
    int *p = malloc(sizeof(int) * 5);
    for (int i = 0; i < 5; i++) p[i] = i * 10;
    int sum = 0;
    for (int i = -1; i < 5; i++)
        sum += p[i];
    printf("%d\n", sum);
    free(p);
    return 0;
}
