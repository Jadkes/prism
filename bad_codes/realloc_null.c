#include <stdlib.h>
#include <string.h>
int main(void) {
    char *p = malloc(10);
    strcpy(p, "hello");
    p = realloc(p, 1000000000);
    strcpy(p, "world");
    free(p);
    return 0;
}
