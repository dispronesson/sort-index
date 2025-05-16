#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>

struct index_s {
    double time_mark;
    uint64_t recno;
};

struct index_hdr_s {
    uint64_t records;
    struct index_s idxs[];
};

int parse_ull(const char* str, uint64_t* out);
double random_mjd(uint64_t curr_val);

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <filename> <records> <mjd>\n", argv[0]);
        fprintf(stderr, "<filename> - file in which unsorted index records will be generated\n");
        fprintf(stderr, "<records> - number of records, multiple of 16, unsigned integer\n");
        fprintf(stderr, "<mjd> - modified julian day, unsigned integer, > 15020\n");
        return 1;
    }

    uint64_t records;
    int res = parse_ull(argv[2], &records);
    if (res == -1) {
        fprintf(stderr, "generator: <records> - not valid number\n");
        return 1;
    }
    if (res == -2) {
        fprintf(stderr, "generator: <records> - overflow\n");
        return 1;
    }
    if (res == -3 || records == 0) {
        fprintf(stderr, "generator: <records> - negative or zero\n");
        return 1;
    }
    if ((records & 0xF) != 0) {
        fprintf(stderr, "generator: <records> - must be a multiple of 16\n");
        return 1;
    }

    uint64_t mjd;
    res = parse_ull(argv[3], &mjd);
    if (res == -1) {
        fprintf(stderr, "generator: <mjd> - not valid number\n");
        return 1;
    }
    if (res == -2) {
        fprintf(stderr, "generator: <mjd> - overflow\n");
        return 1;
    }
    if (res == -3 || mjd == 0) {
        fprintf(stderr, "generator: <mjd> - negative or zero\n");
        return 1;
    }
    if (mjd <= 15020) {
        fprintf(stderr, "generator: <mjd> - less or equal 15020\n");
        return 1;
    }

    int fd = open(argv[1], O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if (fd == -1) {
        perror("generator: open");
        return 1;
    }

    size_t size = records * sizeof(struct index_s) + sizeof(struct index_hdr_s);
    struct index_hdr_s* idx_hdr;
    idx_hdr = malloc(size);
    if (!idx_hdr) {
        perror("generator: malloc");
        close(fd);
        return 1;
    }

    idx_hdr->records = records;

    srandom((unsigned)time(NULL));

    for (uint64_t i = 0; i < records; i++) {
        idx_hdr->idxs[i].recno = i + 1;
        idx_hdr->idxs[i].time_mark = random_mjd(mjd);
    }

    ssize_t written = write(fd, idx_hdr, size);
    if (written != (ssize_t)size) {
        perror("generator: write");
        close(fd);
        free(idx_hdr);
        return 1;
    }

    close(fd);
    free(idx_hdr);
    return 0;
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

double random_mjd(uint64_t curr_val) {
    uint64_t range = curr_val - 15020;
    uint64_t mjd_int = 15020 + (random() % range);
    double mjd_double = ((double)random() / ((unsigned)INT32_MAX + 1));
    return mjd_int + mjd_double;
}