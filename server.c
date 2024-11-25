#include "uring_util.h"
#include "sock_util.h"
#include "misc_util.h"

const char *const enter_name_prompt = "Enter your name: ";

#define BUF_SIZE 4096

int id_counter = 1;
Array clients;

struct Client
{
    int id;
    int fd;
    char name[64];
    bool ready;
};

typedef struct Client Client;

void on_stdin(UringCallbackContext *context, Buffer buf)
{
    if (strcmp(buf.data, "exit\n") == 0)
    {
        uring_stop(context->uring);
        buffer_free(buf);
        return;
    }

    uring_read(context->uring, STDIN_FILENO, buf, 0, on_stdin, 0);
}

void broadcast_message(Uring *uring, Buffer *buf, Client *sender)
{
    for (int i = 0; i < clients.length; i++)
    {
        Client *client = array_get(&clients, i);
        if (sender && client->id == sender->id)
            continue;
        if (!client->ready)
            continue;
        uring_write(uring, client->fd, buffer_copy(buf), 0, uring_null_callback, 0);
    }
}

void handle_client_disconnect(Uring *uring, int client_fd)
{
    printf("Client %d disconnected\n", client_fd);
    uring_close(uring, client_fd, uring_null_callback, 0);

    for (int i = 0; i < clients.length; i++)
    {
        Client *client = array_get(&clients, i);
        if (client->fd == client_fd)
        {
            array_remove(&clients, i);

            Buffer message = buffer_from_string(client->name);
            buffer_append_string(&message, " left the chat\n");
            broadcast_message(uring, &message, client);
            buffer_free(message);

            drop(client);
            return;
        }
    }

    fprintf(stderr, "Client not found\n");
}

void client_read_message(Uring *uring, Client *client, Buffer buf);
void on_client_message(UringCallbackContext *context, Buffer buf)
{
    Client *client = (Client *)context->data;
    int client_fd = client->fd;

    if (buf.length == 0)
    {
        handle_client_disconnect(context->uring, client_fd);
        buffer_free(buf);
        return;
    }

    client_read_message(context->uring, client, buf);

    Buffer send_buf = buffer_from_string(client->name);
    buffer_append_string(&send_buf, ": ");
    strtok(buf.data, "\n");
    strtok(buf.data, "\r");
    buffer_append_string(&send_buf, buf.data);
    buffer_append_string(&send_buf, "\n");

    printf("Client %s: %s\n", client->name, buf.data);

    broadcast_message(context->uring, &send_buf, client);
    buffer_free(send_buf);
}

void client_read_message(Uring *uring, Client *client, Buffer buf)
{
    uring_read(uring, client->fd, buf, 0, on_client_message, (unsigned long long)client);
}

void client_onboarding(Uring *uring, Client *client);
void on_client_enter_name(UringCallbackContext *context, Buffer buf)
{
    Client *client = (Client *)context->data;
    int client_fd = client->fd;

    if (buf.length == 0)
    {
        handle_client_disconnect(context->uring, client_fd);
        return;
    }

    strncpy(client->name, buf.data, sizeof(client->name));

    for (int i = 0; i < strlen(client->name); i++)
    {
        if (client->name[i] == '\n' || client->name[i] == '\r')
        {
            client->name[i] = '\0';
            break;
        }
    }

    if (strlen(client->name) == 0)
    {
        uring_write(context->uring, client_fd, buffer_from_string("Name cannot be empty\n"), 0, uring_null_callback, 0);
        client_onboarding(context->uring, client);
        return;
    }

    printf("Client %d entered name: %s\n", client->id, client->name);

    client->ready = true;
    client_read_message(context->uring, client, buf);

    Buffer message = buffer_from_string(client->name);
    buffer_append_string(&message, " joined the chat\n");
    broadcast_message(context->uring, &message, NULL);
    buffer_free(message);
}

void client_onboarding(Uring *uring, Client *client)
{
    uring_write(uring, client->fd, buffer_from_string("Enter your name: "), 0, uring_null_callback, 0);
    uring_read(uring, client->fd, buffer_build(BUF_SIZE), 0, on_client_enter_name, (unsigned long long)client);
}

void on_client_connect(UringCallbackContext *context)
{
    int client_fd = (int)context->completion->res;
    int server_fd = (int)context->data;
    printf("Accepted client %d\n", client_fd);

    Client *client = new(Client);
    client->fd = client_fd;
    client->id = id_counter++;
    client->ready = false;
    array_push(&clients, client);

    uring_accept(context->uring, server_fd, NULL, 0, on_client_connect, server_fd);
    client_onboarding(context->uring, client);
}

int main(int argc, char *argv[])
{
    array_init(&clients, 4);

    Uring *uring = ptr_or_exit(uring_new(64), "uring_new");

    int port = argc == 2 ? strtol(argv[1], NULL, 10) : 8901;
    int server_fd = fd_or_exit(tcp_server(loopback_address(port)), "tcp_server");
    printf("Listening on port %d\n", port);

    uring_read(uring, STDIN_FILENO, buffer_build(BUF_SIZE), 0, on_stdin, 0);
    uring_accept(uring, server_fd, NULL, 0, on_client_connect, server_fd);

    uring_run(uring);
    drop(uring);
    close(server_fd);

    for (int i = 0; i < clients.length; i++)
    {
        Client *client = array_get(&clients, i);
        close(client->fd);
        drop(client);
    }

    // drop(clients.data);

    printf("Server closed\n");

    allocation_summary();

    return 0;
}
