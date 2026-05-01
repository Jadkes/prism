#include <stdio.h>
#include <string.h>
int main(void) {
    char buf[4];
    snprintf(buf, sizeof(buf), "hello world");
    printf("%s\n", buf);
    return 0;
}
