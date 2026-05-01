#include <stdio.h>
int main(void) {
    long n = 10000000000L;
    int arr[n];
    arr[0] = 42;
    printf("%d\n", arr[0]);
    return 0;
}
