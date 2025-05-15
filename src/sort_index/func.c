#include "func.h"

uint64_t memsize_rounded;
uint64_t blocks;
uint64_t threads;
int fd;

void check_cmd_line(int argc, char** argv) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <memsize> <blocks> <threads> <filename>\n", argv[0]);
        fprintf(stderr, "<memsize> - size of working buffer, multiple of the page size\n");
        fprintf(stderr, "<blocks> - number of blocks to split the buffer, > <threads>\n");
        fprintf(stderr, "<threads> - number of working threads, from k (count of CPUs) to 8 * k\n");
        fprintf(stderr, "<filename> - file for sorting\n");
        exit(EXIT_FAILURE);
    }

    uint64_t memsize;
    int res = parse_ull(argv[1], &memsize);
    if (res == -1) {
        fprintf(stderr, "sort_index: <memsize> - not valid number\n");
        exit(EXIT_FAILURE);
    }
    if (res == -2) {
        fprintf(stderr, "sort_index: <memsize> - overflow\n");
        exit(EXIT_FAILURE);
    }
    if (res == -3 || memsize == 0) {
        fprintf(stderr, "sort_index: <memsize> - negative or zero\n");
        exit(EXIT_FAILURE);
    }

    res = parse_ull(argv[2], &blocks);
    if (res == -1) {
        fprintf(stderr, "sort_index: <blocks> - not valid number\n");
        exit(EXIT_FAILURE);
    }
    if (res == -2) {
        fprintf(stderr, "sort_index: <blocks> - overflow\n");
        exit(EXIT_FAILURE);
    }
    if (res == -3 || blocks == 0) {
        fprintf(stderr, "sort_index: <blocks> - negative or zero\n");
        exit(EXIT_FAILURE);
    }
    if ((blocks & (blocks - 1)) != 0) {
        fprintf(stderr, "sort_index: <blocks> - must be a power of two\n");
        exit(EXIT_FAILURE);
    }

    res = parse_ull(argv[3], &threads);
    if (res == -1) {
        fprintf(stderr, "sort_index: <threads> - not valid number\n");
        exit(EXIT_FAILURE);
    }
    if (res == -2) {
        fprintf(stderr, "sort_index: <threads> - overflow\n");
        exit(EXIT_FAILURE);
    }
    if (res == -3 || threads == 0) {
        fprintf(stderr, "sort_index: <threads> - negative or zero\n");
        exit(EXIT_FAILURE);
    }
    if (threads >= blocks) {
        fprintf(stderr, "sort_index: <threads> - must be < than <blocks>\n");
        exit(EXIT_FAILURE);
    }

    long k = sysconf(_SC_NPROCESSORS_ONLN);
    long n = 8 * k;
    if (threads < k || threads > n) {
        fprintf(stderr, "sort_index: <threads> - must be between %ld and %ld\n", k, n);
        exit(EXIT_FAILURE);
    }

    fd = open(argv[4], O_RDWR);
    if (fd == -1) {
        perror("sort_index: open");
        exit(EXIT_FAILURE);
    }

    int page_size = getpagesize();
    memsize_rounded = ((memsize + page_size - 1) / page_size) * page_size;
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