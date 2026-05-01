#include <string.h>
int main(void) {
    char buf[5];
    strcpy(buf, "this is way too long for the buffer");
    return 0;
}
