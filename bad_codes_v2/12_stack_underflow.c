#include <stdio.h>
int main(void) {
    int buf[5];
    buf[-1] = 42;
    printf("%d\n", buf[-1]);
    return 0;
}
