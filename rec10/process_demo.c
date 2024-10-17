#include <stdio.h>
#include <unistd.h>
#include <assert.h>

int main() {
    int n;
    int rc = scanf("%d", &n);
    assert(rc == 1);

    for (int i = 0; i < n; i++) {
        printf("%d\n", i);
        sleep(1);
    }
    printf("Done.\n");

    return 0;
}
