#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

struct index_s {
    double time_mark;
    uint64_t recno;
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        fprintf(stderr, "<filename> - file that contains index records\n");
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("sortcheck: open");
        return 1;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("sortcheck: fstat");
        close(fd);
        return 1;
    }

    off_t file_size = st.st_size;
    if (file_size < (off_t)sizeof(uint64_t)) {
        fprintf(stderr, "sortcheck: file doesn't contain a header\n");
        close(fd);
        return 1;
    }
    
    uint64_t records;
    ssize_t r = read(fd, &records, sizeof(uint64_t));
    if (r != sizeof(uint64_t)) {
        perror("sortcheck: read header");
        close(fd);
        return 1;
    }

    uint64_t data_size = file_size - sizeof(uint64_t);
    if (data_size == 0) {
        fprintf(stderr, "sortcheck: file doesn't contain index records\n");
        close(fd);
        return 1;
    }
    if ((data_size & 0xFF) != 0) {
        fprintf(stderr, "sortcheck: index records is not multiple of 256\n");
        close(fd);
        return 1;
    }
    if (data_size != records * sizeof(struct index_s)) {
        fprintf(stderr, "sortcheck: data size mismatch with record count\n");
        close(fd);
        return 1;
    }

    struct index_s idxs;
    r = read(fd, &idxs, (off_t)sizeof(struct index_s));
    if (r != sizeof(struct index_s)) {
        perror("sortcheck: read record");
        close(fd);
        return 1;
    }

    double prev = idxs.time_mark;
    for (uint64_t i = 1; i < records; i++) {
        r = read(fd, &idxs, sizeof(struct index_s));
        if (r != sizeof(struct index_s)) {
            perror("sortcheck: read record");
            close(fd);
            return 1;
        }

        if (idxs.time_mark < prev) {
            printf("sortcheck: file is not sorted\n");
            close(fd);
            return 2;
        }

        prev = idxs.time_mark;
    }

    printf("sortcheck: file is sorted correctly\n");
    close(fd);
    return 0;
}