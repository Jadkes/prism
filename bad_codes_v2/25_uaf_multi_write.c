#include <stdio.h>
#include <stdlib.h>
int main(void) {
    char *p = malloc(100);
    free(p);
    p[0] = 'a';
    p[1] = 'b';
    return 0;
}
