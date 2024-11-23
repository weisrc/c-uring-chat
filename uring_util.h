
#ifndef URING_UTIL_H
#define URING_UTIL_H

#include "uring_core.h"

typedef enum io_uring_op UringOp;

struct UringCallbackContext
{
    Uring *uring;
    UringCompletion *completion;
    void *data;
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
    void *data;
};
typedef struct UringContext UringContext;

void uring_read_write(Uring *uring, UringOp op, int fd, void *buf, size_t count, off_t offset, UringCallback callback, void *data)
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

void uring_read(Uring *uring, int fd, void *buf, size_t count, off_t offset, UringCallback callback, void *data)
{
    uring_read_write(uring, IORING_OP_READ, fd, buf, count, offset, callback, data);
}

void uring_write(Uring *uring, int fd, void *buf, size_t count, off_t offset, UringCallback callback, void *data)
{
    uring_read_write(uring, IORING_OP_WRITE, fd, buf, count, offset, callback, data);
}

void uring_start(Uring *uring)
{
    UringCompletion *completion;

    while (1)
    {
        if (uring_empty(uring))
            break;

        if (uring_enter(uring) < 0)
            break;

        while ((completion = uring_completion(uring)) != NULL)
        {
            if (completion->user_data == 0)
                continue;

            UringContext *callback_data = (UringContext *)completion->user_data;
            UringCallback callback = callback_data->callback;
            void *data = callback_data->data;
            free(callback_data);

            UringCallbackContext context = {uring, completion, data};
            callback(&context);
        }
    }
}

#endif