#include <stdio.h>
#include <stdlib.h>
int main(void) {
    int *p = malloc(sizeof(int) * 100);
    int *end = p + 100;
    while (p < end) {
        *p = 0;
        p++;
    }
    *p = 42;
    return 0;
}
