#ifndef SORT_INDEX_H
#define SORT_INDEX_H

#include "valid_args.h"
#include <sys/mman.h>
#include <string.h>
#include <sys/param.h>

typedef struct {
    ctx_t* ctx;
    uint32_t tid;
} thr_arg_t;

void* worker(void* arg);
int load_next_chunk(ctx_t* ctx);
void build_block_map(ctx_t* ctx);
void sort_phase(ctx_t* ctx, uint32_t tid);
int cmp_tm(const void* a, const void* b);
void merge_round(ctx_t* ctx, uint32_t tid, uint64_t cur_blokcs);
void merge_phase(ctx_t* ctx, uint32_t tid);
void merge_final(ctx_t* ctx);

#endif //SORT_INDEX_H