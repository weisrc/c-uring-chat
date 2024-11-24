#include "uring_util.h"
#include "sock_util.h"
#include "misc_util.h"

char buf[4096];

void on_stdin(UringCallbackContext *context)
{
    uring_read(context->uring, STDIN_FILENO, buf, sizeof(buf), 0, on_stdin, 0);
    printf("Read %d bytes\n", context->completion->res);
}

void on_client_data(UringCallbackContext *context)
{
    int client_fd = (int)context->data;
    int bytes_read = context->completion->res;

    if (bytes_read == 0)
    {
        printf("Client %d disconnected\n", client_fd);
        uring_close(context->uring, client_fd, uring_null_callback, 0);
        return;
    }

    printf("Read %d bytes from client %d\n", context->completion->res, client_fd);
    uring_write(context->uring, client_fd, buf, context->completion->res, 0, uring_null_callback, 0);
    uring_read(context->uring, client_fd, buf, sizeof(buf), 0, on_client_data, client_fd);
}

void on_client_connect(UringCallbackContext *context)
{
    int client_fd = (int)context->completion->res;
    int server_fd = (int)context->data;
    printf("Accepted client %d\n", client_fd);
    uring_accept(context->uring, server_fd, NULL, 0, on_client_connect, server_fd);
    uring_read(context->uring, client_fd, buf, sizeof(buf), 0, on_client_data, client_fd);
}

int main(int argc, char *argv[])
{
    Uring *uring = ptr_or_exit(uring_new(8), "uring_new");

    int port = argc == 2 ? strtol(argv[1], NULL, 10) : 8901;
    int server_fd = fd_or_exit(server(loopback_address(port)), "server");
    printf("Listening on port %d\n", port);

    uring_read(uring, STDIN_FILENO, buf, sizeof(buf), 0, on_stdin, 0);
    uring_accept(uring, server_fd, NULL, 0, on_client_connect, server_fd);

    uring_run(uring);
    free(uring);
    close(server_fd);

    printf("Server closed\n");
    return 0;
}