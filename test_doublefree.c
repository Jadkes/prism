#include <stdio.h>
#include <stdlib.h>
int main(void) {
    char *buf = malloc(32);
    free(buf);
    free(buf);
    return 0;
}