#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdbool.h>

static int cnt = 1; // used to be a timer to valid's timestp functionality.

typedef struct {
    int valid;  // we can combine the valid and timestp, in this lab.
    size_t tag;
} cache_line_t;

static cache_line_t** caches = NULL;

void printSummary(int hits,  /* number of hits */
                  int misses, /* number of misses */
                  int evictions); /* number of evictions */

static const char* trace_file_path = NULL;
static int         hits = 0, misses = 0, evictions = 0;
static int         set = -1, E = -1, block = -1, verbose = false;

static void print_usage(const char* exec_name) {
    printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>",
           exec_name);
    puts("Options:");
    puts("  -h         Print this help message.");
    puts("  -v         Optional verbose flag.");
    puts("  -s <num>   Number of set index bits.");
    puts("  -E <num>   Number of lines per set.");
    puts("  -b <num>   Number of block offset bits.");
    puts("  -t <file>  Trace file.");
    puts("Examples:");
    puts("  linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace");
    puts("  linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace");
}

static void parse_cmd(int argc, char** argv){
    int opt = -1;
    while( (opt = getopt(argc, argv, "vs:E:b:t:")) != -1){
        switch(opt){
        case 'v':
            verbose = true;
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
            break;
        case '?':
            print_usage(argv[0]);
            exit(EXIT_FAILURE);
        case 'h':
            print_usage(argv[0]);
            exit(EXIT_SUCCESS);
        }
    }
    if (set <= 0 || E <= 0 || block <= 0 || set + block > 64 || trace_file_path == NULL) {
        exit(EXIT_FAILURE);
    }
}

void init_cache(){
    int set_nums = 1 << set;
    caches = malloc(set_nums * sizeof(cache_line_t*));
    assert(caches);
    for(int i = 0; i < set_nums; i++){
        caches[i] = malloc(E * sizeof(cache_line_t));
        assert(caches[i]);
        memset(caches[i], 0, E * sizeof(cache_line_t));
    }
}

void find_cache(size_t addr){
    size_t tag = addr >> (set + block);
    size_t set_index = (addr >> block) & ((1 << set) - 1);

    int line_index = -1;
    for(int i = 0; i < E; i++){
        if(caches[set_index][i].valid && caches[set_index][i].tag == tag){
            line_index = i;
            break;
        }
    }

    if(line_index != -1){
        hits++;
        caches[set_index][line_index].valid = cnt++;
        return;
    }

    misses++;

    int valid_min = INT_MAX;
    for(int i = 0; i < E; i++){
        if(!caches[set_index][i].valid){
            line_index = i;
            break;
        }
        if(caches[set_index][i].valid < valid_min){
            line_index = i;
            valid_min = caches[set_index][i].valid;
        }
    }

    if(caches[set_index][line_index].valid)
        evictions++;
    caches[set_index][line_index].tag = tag;
    caches[set_index][line_index].valid = cnt++;
}

void read_from_file(const char* trace_file_path) {
    char op[7]; size_t addr; int size;
    FILE* fp = fopen(trace_file_path, "r");

    while(fscanf(fp, "%s %lx, %d\n", op, &addr, &size) == 3){
        if(op[0] == 'I')
            continue;

        if (verbose) {
            printf("%s %lx,%d \n", op, addr, size);
        }

        find_cache(addr);

        if(op[0] == 'M')
            hits++;
    }
}

void free_cache(){
    int set_nums = 1 << set;
    for(int i = 0; i < set_nums; i++){
        free(caches[i]);
    }
    free(caches);
}

int main(int argc, char** argv) {
    parse_cmd(argc, argv);

    init_cache();

    read_from_file(trace_file_path);

    free_cache();

    printSummary(hits, misses, evictions);
    return 0;
}
