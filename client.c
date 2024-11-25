#include "uring_util.h"
#include "sock_util.h"
#include "misc_util.h"

#define BUF_SIZE 4096

void on_stdin(UringCallbackContext *context, Buffer buf)
{
    uring_write(context->uring, context->data, buffer_copy(&buf), 0, uring_null_callback, 0);
    uring_read(context->uring, STDIN_FILENO, buf, 0, on_stdin, context->data);
}

void on_message(UringCallbackContext *context, Buffer buf)
{
    if (buf.length == 0)
    {
        uring_stop(context->uring);
        buffer_free(buf);
        return;
    }

    printf("%s", buf.data);
    uring_read(context->uring, context->data, buf, 0, on_message, context->data);
}

int main(int argc, char *argv[])
{
    Uring *uring = ptr_or_exit(uring_new(64), "uring_new");

    int port = argc == 2 ? strtol(argv[1], NULL, 10) : 8901;
    int client_fd = fd_or_exit(tcp_client(loopback_address(port)), "tcp_client");
    printf("Connected to port %d\n", port);

    uring_read(uring, STDIN_FILENO, buffer_build(BUF_SIZE), 0, on_stdin, client_fd);
    uring_read(uring, client_fd, buffer_build(BUF_SIZE), 0, on_message, client_fd);

    uring_run(uring);
    drop(uring);
    close(client_fd);

    printf("Disconnected\n");
    return 0;
}
