#include <stdio.h>
#include <string.h>
int main(void) {
    char buf[5];
    memcpy(buf, "hello world this is too long", 30);
    printf("%s\n", buf);
    return 0;
}
