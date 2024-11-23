#include "uring_util.h"

char buf[4096];

void read_callback(UringCallbackContext *context)
{
    uring_read(context->uring, STDIN_FILENO, buf, sizeof(buf), 0, read_callback, NULL);
    printf("Read %d bytes\n", context->completion->res);
}

int main(int argc, char *argv[])
{
    Uring uring;

    if (!uring_init(&uring, 1))
    {
        fprintf(stderr, "Unable to setup uring!\n");
        return 1;
    }

    uring_read(&uring, STDIN_FILENO, buf, sizeof(buf), 0, read_callback, NULL);

    uring_start(&uring);
}