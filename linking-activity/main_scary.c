#include <stdio.h>

extern int global;

int main(void) {
    printf("before: %d\n", global);
    global = 15213;
    printf("after: %d\n", global);
    return 0;
}
