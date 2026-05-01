#include <stdlib.h>
int main(void) {
    int *p = malloc(4);
    p[100] = 42;
    free(p);
    return 0;
}
