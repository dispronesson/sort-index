#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

struct index_s {
    double time_mark;
    uint64_t recno;
};

double random_mjd(int curr_val) {
    int range = curr_val - 15020 - 1;
    int mjd_int = 15020 + (random() % range);
    double mjd_double = ((double)random() / ((unsigned)INT32_MAX + 1));
    return mjd_int + mjd_double;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <filename> <count>\n", argv[0]);
        return 1;
    }
    if (argv[2][0] == '-') {
        fprintf(stderr, "Count cannot be negative\n");
        return 1;
    }

    char* endptr;
    uint64_t count = strtoull(argv[2], &endptr, 10);
    if (*endptr != '\0') {
        fprintf(stderr, "Count is not a valid number\n");
        return 1;
    }
    else if (count == 0) {
        fprintf(stderr, "Count cannot be zero\n");
        return 1;
    }
    else if (count % 16 != 0) {
        fprintf(stderr, "Count must be a multiple of 16\n");
        return 1;
    }

    int fd = open(argv[1], O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    srandom(time(NULL));

    ssize_t written = write(fd, &count, sizeof(uint64_t));
    if (written != sizeof(uint64_t)) {
        perror("write count");
        close(fd);
        return 1;
    }

    for (uint64_t i = 0; i < count; i++) {
        struct index_s rec;
        rec.time_mark = random_mjd(60810);
        rec.recno = i + 1;
        write(fd, &rec, sizeof(struct index_s));
        if (written != sizeof(uint64_t)) {
            perror("write record");
            close(fd);
            return 1;
        }
    }

    close(fd);
    return 0;
}