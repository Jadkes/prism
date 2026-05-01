#include <stdio.h>
int main(void) {
    char buf[10];
    sprintf(buf, "Hello %s, your number is %d and more text", "World", 12345);
    return 0;
}
