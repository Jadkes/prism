#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(void) {
    char *p = malloc(10);
    strcpy(p, "hi");
    free(p);
    size_t len = strlen(p);
    printf("%zu\n", len);
    return 0;
}
