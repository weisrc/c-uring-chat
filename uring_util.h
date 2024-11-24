
#ifndef URING_UTIL_H
#define URING_UTIL_H

#include "uring_core.h"
#include <arpa/inet.h>

typedef enum io_uring_op UringOp;

struct UringCallbackContext
{
    Uring *uring;
    UringCompletion *completion;
    unsigned long long data;
};
typedef struct UringCallbackContext UringCallbackContext;

void uring_null_callback(UringCallbackContext *context)
{
    return;
}

typedef void (*UringCallback)(UringCallbackContext *);

struct UringContext
{
    UringCallback callback;
    unsigned long long data;
};
typedef struct UringContext UringContext;

Uring *uring_new(unsigned int queue_length)
{
    Uring *uring = malloc(sizeof(Uring));
    if (!uring_init(uring, queue_length))
    {
        free(uring);
        return NULL;
    }
    return uring;
}

void uring_accept(Uring *uring, int fd, struct sockaddr *addr, socklen_t addrlen, UringCallback callback, unsigned long long data)
{
    UringTask *task = uring_task(uring);

    UringContext *callback_data = malloc(sizeof(UringContext));
    callback_data->callback = callback;
    callback_data->data = data;

    task->opcode = IORING_OP_ACCEPT;
    task->fd = fd;
    task->addr = (unsigned long)addr;
    task->len = (unsigned int)addrlen;
    task->user_data = (unsigned long)callback_data;

    uring_submit(uring);
}

void uring_close(Uring *uring, int fd, UringCallback callback, unsigned long long data)
{
    UringTask *task = uring_task(uring);

    UringContext *callback_data = malloc(sizeof(UringContext));
    callback_data->callback = callback;
    callback_data->data = data;

    task->opcode = IORING_OP_CLOSE;
    task->fd = fd;
    task->user_data = (unsigned long)callback_data;

    uring_submit(uring);
}

void uring_read_write(Uring *uring, UringOp op, int fd, char *buf, size_t count, off_t offset, UringCallback callback, unsigned long long data)
{
    UringTask *task = uring_task(uring);

    UringContext *callback_data = malloc(sizeof(UringContext));
    callback_data->callback = callback;
    callback_data->data = data;

    task->opcode = IORING_OP_READ;
    task->opcode = op;
    task->fd = fd;
    task->addr = (unsigned long)buf;
    task->len = count;
    task->off = offset;
    task->user_data = (unsigned long)callback_data;

    uring_submit(uring);
}

void uring_read(Uring *uring, int fd, char *buf, size_t count, off_t offset, UringCallback callback, unsigned long long data)
{
    uring_read_write(uring, IORING_OP_READ, fd, buf, count, offset, callback, data);
}

void uring_write(Uring *uring, int fd, char *buf, size_t count, off_t offset, UringCallback callback, unsigned long long data)
{
    uring_read_write(uring, IORING_OP_WRITE, fd, buf, count, offset, callback, data);
}

void uring_stop(Uring *uring)
{
    uring->running = false;
}

void uring_run(Uring *uring)
{
    UringCompletion *completion;
    uring->running = true;

    while (uring->running)
    {
        // if (uring_empty(uring))
        // {
        //     printf("Empty\n");
        //     break;
        // }

        if (uring_enter(uring) < 0)
        {
            printf("Enter\n");
            break;
        }

        while ((completion = uring_completion(uring)) != NULL)
        {
            if (completion->user_data == 0)
                continue;

            UringContext *callback_data = (UringContext *)completion->user_data;
            UringCallback callback = callback_data->callback;
            unsigned long long data = callback_data->data;
            free(callback_data);

            UringCallbackContext context = {uring, completion, data};
            callback(&context);
        }
    }
}

#endif