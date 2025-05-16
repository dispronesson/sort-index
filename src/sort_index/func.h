#ifndef FUNC_H
#define FUNC_H

#define _DEFAULT_SOURCE

#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

void validate_args(int argc, char** argv);
int parse_ull(const char* str, uint64_t* out);

#endif //FUNC_H