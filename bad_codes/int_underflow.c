#include <stdio.h>
#include <limits.h>
int main(void) {
    int x = INT_MIN;
    x = x - 1;
    printf("%d\n", x);
    return 0;
}
