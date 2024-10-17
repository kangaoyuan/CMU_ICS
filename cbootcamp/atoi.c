#include <stdio.h>
#include <stdlib.h>

int main() {
    char *this_course = "15213";
    char bad[3] = { 'b', 'a', 'd' };
    char *zero = "0";
    // It's recommend to use strtol();

    printf("atoi(this_course): %d\n", atoi(this_course));
    perror("first:");
    // No null-terminated
    printf("atoi(bad): %d\n", atoi(bad));
    perror("second:");
    printf("atoi(zero): %d\n", atoi(zero));
    perror("third:");
    // from above perror() output, we find it can't distinguish the error and 0, due to the return val 0 on error.

    return 0;
}
