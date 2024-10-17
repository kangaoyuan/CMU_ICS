#include <stdio.h>

extern int global;

void set_global(int val);

int main(void) {
    printf("before: %d\n", global);
    set_global(15213);
    printf("after: %d\n", global);
    return 0;
}
