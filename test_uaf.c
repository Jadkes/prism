#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(void) {
    char *buf = malloc(32);
    strcpy(buf, "hello");
    free(buf);
    printf("%s\n", buf);
    return 0;
}