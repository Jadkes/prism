#include <stdlib.h>
int main(void) {
    int x = 42;
    free(&x);
    return 0;
}
