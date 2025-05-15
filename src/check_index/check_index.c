#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

struct index_s {
    double time_mark;
    uint64_t recno;
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    uint64_t count;
    ssize_t readed = read(fd, &count, sizeof(uint64_t));
    if (readed != sizeof(uint64_t)) {
        fprintf(stderr, "Bad file\n");
        close(fd);
        return 1;
    }

    struct index_s prev, curr;
    readed = read(fd, &prev, sizeof(struct index_s));
    if (readed != sizeof(struct index_s)) {
        fprintf(stderr, "Bad file\n");
        close(fd);
        return 1;
    }

    for (uint64_t i = 0; i < count - 1; i++) {
        readed = read(fd, &curr, sizeof(struct index_s));
        if (readed != sizeof(struct index_s)) {
            fprintf(stderr, "Bad file\n");
            close(fd);
            return 1;
        }

        if (curr.time_mark < prev.time_mark) {
            printf("File is not sorted\n");
            close(fd);
            return 2;
        }

        prev = curr;
    }

    printf("File is sorted correctly\n");
    close(fd);
    return 0;
}