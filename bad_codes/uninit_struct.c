#include <stdio.h>
struct Data { int a; int b; int c; };
int main(void) {
    struct Data d;
    printf("%d %d %d\n", d.a, d.b, d.c);
    return 0;
}
