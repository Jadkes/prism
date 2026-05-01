int *get_ptr(void) {
    int x = 42;
    return &x;
}
int main(void) {
    int *p = get_ptr();
    return *p;
}
