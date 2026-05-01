#include <stdio.h>
#include <stdlib.h>
int main(void) {
    int **matrix = malloc(sizeof(int*) * 5);
    for (int i = 0; i < 5; i++)
        matrix[i] = malloc(sizeof(int) * 5);
    /* forgot to free inner arrays */
    free(matrix);
    return 0;
}
