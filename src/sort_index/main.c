#include "sort_index.h"

int main(int argc, char* argv[]) {
    ctx_t* ctx = validate_args(argc, argv);

    pthread_t tids[ctx->threads - 1];
    thr_arg_t targ[ctx->threads];

    for (uint64_t i = 1; i < ctx->threads; i++) {
        targ[i].ctx = ctx;
        targ[i].tid = i;

        int rc = pthread_create(&tids[i - 1], NULL, worker, &targ[i]);
        if (rc != 0) {
            errno = rc;
            perror("sortindex: pthread_create");
            close(ctx->fd);
            free(ctx->map);
            free(ctx);
            exit(EXIT_FAILURE);
        }
    }

    targ[0].ctx = ctx;
    targ[0].tid = 0;
    worker(&targ[0]);

    for (uint64_t i = 1; i < ctx->threads; i++) {
        pthread_join(tids[i - 1], NULL);
    }

    close(ctx->fd);
    pthread_mutex_destroy(&ctx->map_mtx);
    pthread_barrier_destroy(&ctx->barrier);

    free(ctx->map);
    free(ctx);

    return 0;
}