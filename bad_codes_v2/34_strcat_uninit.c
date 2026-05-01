#include <stdio.h>
#include <string.h>
int main(void) {
    char src[100];
    memset(src, 'A', 100);
    src[99] = '\0';
    char dst[10];
    strcat(dst, src);
    return 0;
}
