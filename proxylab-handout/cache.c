#include "csapp.h"
#include "cache.h"

static pthread_cond_t cond;
static pthread_mutex_t mutex;

static struct cache_t caches[MAX_CACHE_SIZE];
static int read_cnt, write_cnt, timestp, cache_size;

void cache_init(){
    pthread_cond_init(&cond, NULL);
    pthread_mutex_init(&mutex, NULL);
}

bool cache_src(rio_t* client_io, const char* uri){
    bool flag = false;

    pthread_mutex_lock(&mutex);
    read_cnt++;
    while(write_cnt > 0){
        pthread_cond_wait(&cond, &mutex);
    }
    pthread_mutex_unlock(&mutex);

    for(int i = 0; i < cache_size; ++i){
        if(!strcmp(uri, caches[i].url)){
            flag = true;
            pthread_mutex_lock(&mutex);
            caches[i].timestp = timestp++;
            pthread_mutex_unlock(&mutex);
            Rio_writen(client_io->rio_fd, caches[i].content, caches[i].size);
            break;
        }
    }

    pthread_mutex_lock(&mutex);
    read_cnt--;
    if(!read_cnt){
        pthread_cond_signal(&cond);
    }
    pthread_mutex_unlock(&mutex);


    return flag ? true : false;
}

void cache_add(const char* url, const char* cache_content, uint cache_n){
    pthread_mutex_lock(&mutex);
    while(read_cnt > 0 || write_cnt > 0){
        pthread_cond_wait(&cond, &mutex);
    }
    write_cnt++;
    pthread_mutex_unlock(&mutex);

    if(cache_size < MAX_CACHE_SIZE){
        strcpy(caches[cache_size].url, url);
        memcpy(caches[cache_size].content, cache_content, cache_n);
        caches[cache_size].size = cache_n;
        pthread_mutex_lock(&mutex);
        caches[cache_size].timestp = timestp++;
        pthread_mutex_unlock(&mutex);
        cache_size++;
    } else {
        uint index = 0, old_time = timestp;
        for (uint i = 0; i < MAX_CACHE_SIZE; ++i) {
            if (caches[i].timestp < old_time) {
                old_time = caches[i].timestp;
                index = i;
            }
        }
        strcpy(caches[index].url, url);
        memcpy(caches[index].content, cache_content, cache_n);
        caches[index].size = cache_n;
        pthread_mutex_lock(&mutex);
        caches[index].timestp = timestp++;
        pthread_mutex_unlock(&mutex);
    }

    pthread_mutex_lock(&mutex);
    write_cnt--;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);
}
