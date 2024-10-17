#include "csapp.h" // IWYU pragma: keep
#include <stdbool.h>
#define URILEN 1024
#define MAX_CACHE_SIZE 10
#define MAX_OBJECT_SIZE 102400

struct cache_t{
    char url[URILEN];
    uint size, timestp;
    char content[MAX_OBJECT_SIZE];
};

void cache_init();
bool cache_src(rio_t* client_io, const char* uri);
void cache_add(const char* url, const char* cache, uint cache_n);
