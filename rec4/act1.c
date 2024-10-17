#include <stdio.h>

int main(int argc, char** argv) {
    /*
     *Upon successful return, these functions return the number of
     *bytes printed (excluding the null byte used to end output to strings).
     */

    int ret = printf("%s\n", argv[argc - 1]);
    argv[0] = '\0';  // NOOP to force gcc to generate a callq instead of jmp
    return ret;
}
