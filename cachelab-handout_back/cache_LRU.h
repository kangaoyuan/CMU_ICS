#ifndef CSAPP_LRU_H
#define CSAPP_LRU_H

#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct node {
    int key;
    // Attention here: for pointer definition.
    struct node *prev, *next;
} node;

// It's called hash, the purpose is to get the set address.
typedef struct hash {
    struct hash* next;         // link data structure.
    struct node* node;         // storing content.
} hash;

typedef struct LRU_Cache {
    int size, capacity;
    hash* table;
    node* head, *tail;
} LRU_Cache;

inline hash* hash_map(hash* table, int key, int capacity){
    int index = key % capacity;
    return table + index;
}

inline void LRU_insert(node* head, node* cur) {
    node* first = head->next;

    if (cur->prev || cur->next) {
        cur->prev->next = cur->next;
        cur->next->prev = cur->prev;
    }

    cur->prev = head;
    cur->next = first;

    head->next = cur;
    first->prev = cur;

    assert(head->next->prev == head);
    assert(first->next->prev == first);
}

inline LRU_Cache* Cache_create(int capacity){
    LRU_Cache* cache = (LRU_Cache*)malloc(sizeof(LRU_Cache));
    cache->table = (hash*)malloc(capacity * sizeof(hash));
    memset(cache->table, 0, capacity*sizeof(hash));
    cache->head = (node*)malloc(sizeof(node));
    memset(cache->head, 0, sizeof(node));
    cache->tail = (node*)malloc(sizeof(node));
    memset(cache->tail, 0, sizeof(node));

    // Not recycle, but double link.
    cache->head->prev = NULL;
    cache->head->next = cache->tail;
    cache->tail->next = NULL;
    cache->tail->prev = cache->head;

    cache->size = 0;
    cache->capacity = capacity;
    return cache;
}

inline int Cache_Get(LRU_Cache* cache, int key){
    hash* addr = hash_map(cache->table, key, cache->capacity);
    addr = addr->next;

    while(addr && addr->node && addr->node->key != key)
        addr = addr->next;

    if(!addr || !addr->node) {
        return 0;
    } else {
        LRU_insert(cache->head, addr->node);
        return 1;
    }

    return 0;
}

inline int Cache_Put(LRU_Cache* cache, int key){
    // addr is only a header node.
    hash* addr = hash_map(cache->table, key, cache->capacity);
    if(cache->size != cache->capacity){
        hash* hash_node = (hash*)malloc(sizeof(hash));
        hash_node->next = addr->next;
        addr->next = hash_node;

        hash_node->node->key = key;
        hash_node->node->prev = hash_node->node->next = NULL;
        LRU_insert(cache->head, hash_node->node);

        ++cache->size;
        return 1;
    } else {
        node* lst = cache->tail->prev;
        hash* del = hash_map(cache->table, lst->key, cache->capacity);

        hash* pre_del = del;
        del = del->next;
        while (del->node->key != lst->key) {
            pre_del = del;
            del = del->next;
        }
        pre_del->next = del->next;

        del->next = addr->next;
        addr->next = del;

        del->node->key = key;
        LRU_insert(cache->head, del->node);

        return 0;
        }
}

#endif
