#include <stdio.h>
#include <stdlib.h>
int main(void) {
    int n = -5;
    int *arr = malloc(sizeof(int) * n);
    if (arr) arr[0] = 42;
    free(arr);
    return 0;
}
