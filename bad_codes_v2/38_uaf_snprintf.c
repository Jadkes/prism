#include <stdio.h>
#include <stdlib.h>
int main(void) {
    char *p = malloc(20);
    snprintf(p, 20, "hello");
    free(p);
    snprintf(p, 20, "world");
    return 0;
}
