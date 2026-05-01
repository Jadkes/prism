#include <stdio.h>
int main(void) {
    int x = 1;
    x = x << 32;
    printf("%d\n", x);
    return 0;
}
