#include "sort_index.h"

void* worker(void* arg) {
    thr_arg_t* targ = arg;
    ctx_t* ctx = targ->ctx;
    uint32_t tid = targ->tid;

    while (1) {
        if (tid == 0) {
            if (load_next_chunk(ctx) == 0) {
                pthread_barrier_wait(&ctx->barrier);
                break;
            }
            build_block_map(ctx);
        }

        pthread_barrier_wait(&ctx->barrier);
        if (tid != 0 && ctx->buf == NULL) break;

        sort_phase(ctx, tid);
        merge_phase(ctx, tid);

        if (tid == 0) {
            msync(ctx->buf, ctx->rec_total * sizeof(index_s), MS_SYNC);
            munmap(ctx->map_base, ctx->map_len);
            ctx->buf = NULL;
        }
        pthread_barrier_wait(&ctx->barrier);
    }

    if (tid == 0) merge_final(ctx);

    return NULL;
}

int load_next_chunk(ctx_t* ctx) {
    if (ctx->file_off >= ctx->file_end) return 0;

    off_t left = ctx->file_end - ctx->file_off;
    size_t bytes = left > (off_t)ctx->memsize ? ctx->memsize : (size_t)left;

    long page_size = sysconf(_SC_PAGE_SIZE);
    off_t aligned_off = ctx->file_off & ~(page_size - 1);
    off_t offset_diff = ctx->file_off - aligned_off;
    size_t map_len = bytes + offset_diff;

    void* map = mmap(NULL, map_len, PROT_READ | PROT_WRITE, MAP_SHARED, ctx->fd, aligned_off);
    if (map == MAP_FAILED) {
        perror("sort_index: mmap");
        exit(EXIT_FAILURE);
    }

    ctx->buf = (index_s*)((char*)map + offset_diff);
    ctx->map_base = map;
    ctx->map_len = map_len;

    ctx->rec_total = bytes / sizeof(index_s);
    ctx->file_off += bytes;
    return 1;
}

void build_block_map(ctx_t* ctx) {
    size_t rec = ctx->rec_total;
    uint64_t blocks = ctx->blocks;

    size_t per_block = rec / blocks;
    size_t extra = rec % blocks;

    size_t off = 0;
    for (uint64_t i = 0; i < blocks; i++) {
        size_t cnt = per_block + (i < extra ? 1 : 0);
        ctx->map[i].ptr = ctx->buf + off;
        ctx->map[i].size = cnt;
        ctx->map[i].busy = (cnt == 0 ? 2 : ((i < ctx->threads) ? 1 : 0));
        off += cnt;
    }
}

void sort_phase(ctx_t* ctx, uint32_t tid) {
    if (ctx->map[tid].size > 1) {
        qsort(ctx->map[tid].ptr, ctx->map[tid].size, sizeof(index_s), cmp_tm);
    }

    while (1) {
        int my = -1;

        pthread_mutex_lock(&ctx->map_mtx);
        for (uint64_t i = ctx->threads; i < ctx->blocks; i++) {
            if (ctx->map[i].busy == 0) {
                ctx->map[i].busy = 1;
                my = i;
                break;
            }
        }
        pthread_mutex_unlock(&ctx->map_mtx);

        if (my == -1) break;

        if (ctx->map[my].size > 1) {
            qsort(ctx->map[my].ptr, ctx->map[my].size, sizeof(index_s), cmp_tm);
        }
    }

    pthread_barrier_wait(&ctx->barrier);
}

int cmp_tm(const void* a, const void* b) {
    double ta = ((const index_s*)a)->time_mark;
    double tb = ((const index_s*)b)->time_mark;
    return (ta > tb) - (ta < tb);
}

void merge_round(ctx_t* ctx, uint32_t tid, uint64_t cur_blokcs) {
    uint64_t pairs = cur_blokcs / 2;
    if (tid >= pairs) {
        pthread_barrier_wait(&ctx->barrier);
        return;
    }

    uint32_t left = tid * 2;
    uint32_t right = left + 1;

    index_s* a = ctx->map[left].ptr;
    size_t n = ctx->map[left].size;
    index_s* b = ctx->map[right].ptr;
    size_t m = ctx->map[right].size;

    index_s* tmp = malloc((n + m) * sizeof(index_s));

    size_t i = 0, j = 0, k = 0;
    while (i < n && j < m) {
        tmp[k++] = (a[i].time_mark <= b[j].time_mark) ? a[i++] : b[j++];
    }
    while (i < n) tmp[k++] = a[i++];
    while (j < m) tmp[k++] = b[j++];

    memcpy(a, tmp, (n + m) * sizeof(index_s));

    ctx->map[left].size = n + m;
    
    free(tmp);

    pthread_barrier_wait(&ctx->barrier);
}

void merge_phase(ctx_t* ctx, uint32_t tid) {
    if (tid == 0) ctx->merge_blocks = ctx->blocks;

    pthread_barrier_wait(&ctx->barrier);

    while (ctx->merge_blocks > 1) {
        merge_round(ctx, tid, ctx->merge_blocks);

        if (tid == 0) {
            uint32_t write = 0;
            for (uint32_t i = 0; i < ctx->merge_blocks; i += 2) {
                ctx->map[write] = ctx->map[i];
                write++;
            }
            ctx->merge_blocks = write;
        }

        pthread_barrier_wait(&ctx->barrier);
    }
}

void merge_final(ctx_t* ctx) {
    size_t full = ctx->file_end - 8;
    size_t step = ctx->memsize; 
    long page_size = sysconf(_SC_PAGE_SIZE);

    while (step < full) {
        off_t left_off = 8;

        while (left_off + (off_t)step < ctx->file_end) {
            off_t right_off = left_off + step;

            size_t left_bytes = step;
            size_t right_bytes = MIN(step, ctx->file_end - right_off);

            off_t left_align = left_off & ~(page_size - 1);
            off_t right_align = right_off & ~(page_size - 1);
            off_t left_shift = left_off - left_align;
            off_t right_shift = right_off - right_align;

            size_t left_map_len = left_bytes + left_shift;
            size_t right_map_len = right_bytes + right_shift;

            void* left_map = mmap(NULL, left_map_len, PROT_READ, MAP_PRIVATE, ctx->fd, left_align);
            if (left_map == MAP_FAILED) {
                perror("sort_index: mmap left");
                exit(EXIT_FAILURE);
            }

            void* right_map = mmap(NULL, right_map_len, PROT_READ, MAP_PRIVATE, ctx->fd, right_align);
            if (right_map == MAP_FAILED) {
                perror("sort_index: mmap right");
                munmap(left_map, left_map_len);
                exit(EXIT_FAILURE);
            }

            index_s* left = (index_s*)((char*)left_map + left_shift);
            index_s* right = (index_s*)((char*)right_map + right_shift);

            size_t left_rec = left_bytes / sizeof(index_s);
            size_t right_rec = right_bytes / sizeof(index_s);
            size_t i = 0, j = 0;

            index_s* out = malloc(left_bytes + right_bytes);
            if (!out) {
                perror("sort_index: malloc");
                exit(EXIT_FAILURE);
            }
            size_t k = 0;

            while (i < left_rec && j < right_rec) {
                out[k++] = (left[i].time_mark <= right[j].time_mark) ? left[i++] : right[j++];
            }
            while (i < left_rec) out[k++] = left[i++];
            while (j < right_rec) out[k++] = right[j++];

            pwrite(ctx->fd, out, left_bytes + right_bytes, left_off);

            free(out);
            munmap(left_map, left_map_len);
            munmap(right_map, right_map_len);

            left_off += step * 2;
        }

        step *= 2;
    }
}