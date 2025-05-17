#ifndef VALID_ARGS_H
#define VALID_ARGS_H

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>
#include <pthread.h>

typedef struct {
    double time_mark;
    uint64_t recno;
} index_s;

typedef struct {
    uint32_t busy;
    uint32_t size;
    index_s* ptr;
} block_t;

typedef struct {
    int fd;
    size_t memsize;
    uint64_t blocks;
    uint64_t threads;
    uint64_t merge_blocks;

    off_t file_off;
    off_t file_end;
    index_s* buf;
    size_t rec_total;

    block_t* map;
    void* map_base;
    size_t map_len;

    pthread_barrier_t barrier;
    pthread_mutex_t map_mtx;
} ctx_t;

ctx_t* validate_args(int argc, char** argv);
int parse_ull(const char* str, uint64_t* out);
void parse_err(const char* str, int opcode);
off_t validate_file(int fd);
ctx_t* ctx_init(size_t memsize, uint64_t blocks, uint64_t threads, int fd, off_t file_end);

#endif //VALID_ARGS_H