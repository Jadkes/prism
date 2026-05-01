#include <stdio.h>
int main(void) {
    int x = 10;
    float *f = (float*)&x;
    *f = 3.14f;
    printf("%d\n", x);
    return 0;
}
