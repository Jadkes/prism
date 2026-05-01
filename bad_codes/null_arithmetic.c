#include <stdio.h>
int main(void) {
    int *p = NULL;
    p = p + 10;
    printf("%p\n", (void*)p);
    return 0;
}
