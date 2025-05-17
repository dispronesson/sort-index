#include "valid_args.h"

ctx_t* validate_args(int argc, char** argv) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <memsize> <blocks> <threads> <filename>\n", argv[0]);
        fprintf(stderr, "<memsize> - size of working buffer, multiple of the page size, unsigned integer\n");
        fprintf(stderr, "<blocks> - number of blocks to split the buffer, > <threads>, unsigned integer\n");
        fprintf(stderr, "<threads> - number of working threads, from k (count of CPUs) to 8 * k, unsigned integer\n");
        fprintf(stderr, "<filename> - file for sorting\n");
        exit(EXIT_FAILURE);
    }

    uint64_t memsize;
    int res = parse_ull(argv[1], &memsize);
    if (res == -1) parse_err("memsize", 0);
    if (res == -2) parse_err("memsize", 1);
    if (res == -3 || memsize == 0) parse_err("memsize", 2);

    uint64_t blocks;
    res = parse_ull(argv[2], &blocks);
    if (res == -1) parse_err("blocks", 0);
    if (res == -2) parse_err("blocks", 1);
    if (res == -3 || blocks == 0) parse_err("blocks", 2);
    if ((blocks & (blocks - 1)) != 0) {
        fprintf(stderr, "sort_index: <blocks> - must be a power of two\n");
        exit(EXIT_FAILURE);
    }

    uint64_t threads;
    res = parse_ull(argv[3], &threads);
    if (res == -1) parse_err("threads", 0);
    if (res == -2) parse_err("threads", 1);
    if (res == -3 || threads == 0) parse_err("threads", 2);

    uint64_t k = (uint64_t)sysconf(_SC_NPROCESSORS_ONLN);
    uint64_t n = 8 * k;
    if (threads < k || threads > n) {
        fprintf(stderr, "sort_index: <threads> - must be between %ld and %ld\n", k, n);
        exit(EXIT_FAILURE);
    }
    if (threads >= blocks) {
        fprintf(stderr, "sort_index: <threads> - must be < than <blocks>\n");
        exit(EXIT_FAILURE);
    }

    int fd = open(argv[4], O_RDWR);
    if (fd == -1) {
        perror("sort_index: open");
        exit(EXIT_FAILURE);
    }

    off_t file_end = validate_file(fd);
    if (file_end < 0) {
        close(fd);
        exit(EXIT_FAILURE);
    }

    long page_size = sysconf(_SC_PAGE_SIZE);
    size_t memsize_rounded = (((size_t)memsize + page_size - 1) & ~(page_size - 1));

    return ctx_init(memsize_rounded, blocks, threads, fd, file_end);
}

int parse_ull(const char* str, uint64_t* out) {
    if (!str) return -1;

    while (isspace(*str)) str++;
    if (*str == '-') return -3;
    if (*str == '\0') return -1;

    errno = 0;
    char* end;
    unsigned long long val = strtoull(str, &end, 10);

    if (*end != '\0') return -1;
    if (errno == ERANGE) return -2;

    *out = (uint64_t)val;
    return 0;
}

void parse_err(const char* str, int opcode) {
    switch (opcode) {
        case 0:
            fprintf(stderr, "sort_index: <%s> - not valid number\n", str);
            exit(EXIT_FAILURE);
        case 1:
            fprintf(stderr, "sort_index: <%s> - overflow\n", str);
            exit(EXIT_FAILURE);
        case 2:
            fprintf(stderr, "sort_index: <%s> - negative or zero\n", str);
            exit(EXIT_FAILURE);
        default:
            fprintf(stderr, "sort_index: <%s> - undefined error\n", str);
            exit(EXIT_FAILURE);
    }
}

off_t validate_file(int fd) {
    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("sort_index: fstat");
        return -1;
    }

    off_t file_size = st.st_size;
    if (file_size < (off_t)sizeof(uint64_t)) {
        fprintf(stderr, "sort_index: file doesn't contain a header\n");
        return -1;
    }

    uint64_t records;
    ssize_t r = read(fd, &records, sizeof(uint64_t));
    if (r != sizeof(uint64_t)) {
        perror("sort_index: read header");
        return -1;
    }

    uint64_t data_size = file_size - sizeof(uint64_t);
    if (data_size == 0) {
        fprintf(stderr, "sort_index: file doesn't contain index records\n");
        return -1;
    }
    if ((data_size & 0xFF) != 0) {
        fprintf(stderr, "sort_index: index records is not multiple of 256\n");
        return -1;
    }
    if (data_size != records * sizeof(index_s)) {
        fprintf(stderr, "sort_index: data size mismatch with record count\n");
        return -1;
    }

    return file_size;
}

ctx_t* ctx_init(size_t memsize, uint64_t blocks, uint64_t threads, int fd, off_t file_end) {
    ctx_t* ctx = malloc(sizeof(ctx_t));
    if (!ctx) {
        perror("sort_index: malloc");
        exit(EXIT_FAILURE);
    }

    ctx->memsize = memsize;
    ctx->blocks = blocks;
    ctx->threads = threads;
    ctx->fd = fd;

    ctx->buf = NULL;
    ctx->file_off = 8;
    ctx->file_end = file_end;
    
    ctx->map = malloc(blocks * sizeof(block_t));
    if (!ctx->map) {
        perror("sort_index: malloc");
        exit(EXIT_FAILURE);
    }

    pthread_barrier_init(&ctx->barrier, NULL, threads);
    pthread_mutex_init(&ctx->map_mtx, NULL);

    return ctx;
}
