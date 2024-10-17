#include "cachelab.h"
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <stdbool.h>


struct cache_t{
    uint64_t valid;
    uint64_t tag;
    uint64_t timestp;
};
struct cache_t** caches;
int hit = 0, miss = 0, evict = 0;

void free_cache(int set){
    for(int i = 0; i < set; ++i)
        free(caches[i]);
    free(caches);
}
void init_cache(int set, int E){
    set = 1 << set;

    caches = malloc(set * sizeof(struct cache_t*));
    if(!caches)
        exit(-1);

    for(int i = 0; i < set; ++i){
        caches[i] = malloc(E * sizeof(struct cache_t));
        if(!caches[i])
            exit(-1);
        for(int j = 0; j < E; ++j){
            caches[i][j].tag = 0;
            caches[i][j].valid = 0;
            caches[i][j].timestp = 0;
        }
    }
    return;
}

void find_cache(uint64_t tag, struct cache_t* cache_lines, int E) {
    bool flag = false;
    for (int i = 0; i < E; ++i) {
        if (!cache_lines[i].valid)
            continue;

        if (cache_lines[i].tag == tag) {
            flag = true;
            cache_lines[i].timestp++;
            break;
        }
    }

    if (flag) {
        hit++;
        return;
    }

    miss++;
    for (int i = 0; i < E; ++i) {
        if (!cache_lines[i].valid) {
            flag = true;
            cache_lines[i].tag = tag;
            cache_lines[i].valid = true;
            break;
        }
    }
    if (flag)
        return;

    evict++;
    uint64_t timestp = UINT_MAX, index = -1;
    for (int i = 0; i < E; ++i) {
        if (cache_lines[i].timestp < timestp) {
            index = i;
            timestp = cache_lines[i].timestp;
            break;
        }
    }
    cache_lines[index].timestp++;
    cache_lines[index].tag = tag;
    cache_lines[index].valid = true;
    return;
}

int main(int argc, char** argv) {
    char * trace_file_path = NULL;
    int opt = -1, set = -1, E = -1, block = -1, verbose = false;
    while( (opt = getopt(argc, argv, "vs:E:b:t:")) != -1){
        switch(opt){
        case 'v':
            verbose = true;
            break;
        case 'h':
            break;
        case 's':
            set = atoi(optarg);
            break;
        case 'E':
            E = atoi(optarg);
            break;
        case 'b':
            block = atoi(optarg);
            break;
        case 't':
            trace_file_path = optarg;
        case '?':
            return 0;
        }
    }
    if (set <= 0 || E <= 0 || block <= 0 || set + block > 64 || trace_file_path == NULL) {
        exit(1);
    }

    init_cache(set, E);

    char op[3]; uint64_t addr; int size;
    FILE* fp = fopen(trace_file_path, "r");

    while(fscanf(fp, "%s %lx, %d", op, &addr, &size) != 3){
        if (verbose) {
            printf("%s %lx,%d ", op, addr, size);
        }

        if(op[0] == 'I')
            continue;

        int tag = addr >> (block+set);
        int set_index = (addr >> block) & ((1 << set) - 1);

        find_cache(tag, caches[set_index], E);

        if(op[0] == 'M')
            find_cache(tag, caches[set_index], E);
    }

    free_cache(set);

    printSummary(hit, miss, evict);
    return 0;
}
