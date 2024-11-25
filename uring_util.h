
#ifndef URING_UTIL_H
#define URING_UTIL_H

#include "uring_core.h"
#include "misc_util.h"
#include <arpa/inet.h>

typedef enum io_uring_op UringOp;

typedef unsigned long long UnsafePointer;

struct UringCallbackContext
{
    Uring *uring;
    UringCompletion *completion;
    UnsafePointer data;
};
typedef struct UringCallbackContext UringCallbackContext;

void uring_null_callback(UringCallbackContext *context)
{
    return;
}

typedef void (*UringCallback)(UringCallbackContext *);
typedef void (*UringReadCallback)(UringCallbackContext *, Buffer);

struct UringCallbackData
{
    UringCallback callback;
    UnsafePointer data;
};
typedef struct UringCallbackData UringCallbackData;

struct UringCallbackDataBuffer
{
    UnsafePointer callback;
    UnsafePointer data;
    Buffer buf;
};

typedef struct UringCallbackDataBuffer UringCallbackDataBuffer;

Uring *uring_new(unsigned int queue_length)
{
    Uring *uring = new(Uring);
    if (!uring_init(uring, queue_length))
    {
        drop(uring);
        return NULL;
    }
    return uring;
}

void uring_accept(Uring *uring, int fd, struct sockaddr *addr, socklen_t addrlen, UringCallback callback, UnsafePointer data)
{
    UringTask *task = uring_task(uring);

    UringCallbackData *callback_data = new(UringCallbackData);
    callback_data->callback = callback;
    callback_data->data = data;

    task->opcode = IORING_OP_ACCEPT;
    task->fd = fd;
    task->addr = (unsigned long)addr;
    task->len = (unsigned int)addrlen;
    task->user_data = (unsigned long)callback_data;

    uring_submit(uring);
}

void uring_close(Uring *uring, int fd, UringCallback callback, UnsafePointer data)
{
    UringTask *task = uring_task(uring);

    UringCallbackData *callback_data = new(UringCallbackData);
    callback_data->callback = callback;
    callback_data->data = data;

    task->opcode = IORING_OP_CLOSE;
    task->fd = fd;
    task->user_data = (unsigned long)callback_data;

    uring_submit(uring);
}

void uring_read_write(Uring *uring, UringOp op, int fd, Buffer buf, off_t offset, UringCallback callback, UnsafePointer data)
{
    UringTask *task = uring_task(uring);

    UringCallbackData *callback_data = new(UringCallbackData);
    callback_data->callback = callback;
    callback_data->data = data;

    task->opcode = op;
    task->fd = fd;
    task->addr = (unsigned long)buf.data;
    task->len = op == IORING_OP_READ ? buf.capacity : buf.length;
    task->off = offset;
    task->user_data = (unsigned long)callback_data;

    uring_submit(uring);
}

void uring_read_callback(UringCallbackContext *context)
{
    Uring *uring = context->uring;
    UringCompletion *completion = context->completion;
    UringCallbackDataBuffer *callback_data = (UringCallbackDataBuffer *)context->data;

    UringReadCallback callback = (UringReadCallback)callback_data->callback;
    UnsafePointer data = callback_data->data;
    Buffer buf = callback_data->buf;
    buf.length = completion->res;
    drop(callback_data);

    UringCallbackContext new_context = {uring, completion, data};
    callback(&new_context, buf);
}

void uring_read(Uring *uring, int fd, Buffer buf, off_t offset, UringReadCallback callback, UnsafePointer data)
{
    UringCallbackDataBuffer *callback_data = new(UringCallbackDataBuffer);
    callback_data->callback = (UnsafePointer)callback;
    callback_data->data = data;
    callback_data->buf = buf;

    uring_read_write(uring, IORING_OP_READ, fd, buf, offset, uring_read_callback, (UnsafePointer)callback_data);
}

void uring_write_callback(UringCallbackContext *context)
{
    Uring *uring = context->uring;
    UringCompletion *completion = context->completion;
    UnsafePointer data = context->data;

    UringCallbackDataBuffer *callback_data = (UringCallbackDataBuffer *)data;
    UringCallback callback = (UringCallback)callback_data->callback;
    data = callback_data->data;
    Buffer buf = callback_data->buf;
    drop(callback_data);

    UringCallbackContext new_context = {uring, completion, data};
    callback(&new_context);
    buffer_free(buf);
}

void uring_write(Uring *uring, int fd, Buffer buf, off_t offset, UringCallback callback, UnsafePointer data)
{
    UringCallbackDataBuffer *callback_data = new(UringCallbackDataBuffer);
    callback_data->callback = (UnsafePointer)callback;
    callback_data->data = data;
    callback_data->buf = buf;
    uring_read_write(uring, IORING_OP_WRITE, fd, buf, offset, uring_write_callback, (UnsafePointer)callback_data);
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

            UringCallbackData *callback_data = (UringCallbackData *)completion->user_data;
            UringCallback callback = callback_data->callback;
            UnsafePointer data = callback_data->data;
            drop(callback_data);

            UringCallbackContext context = {uring, completion, data};
            callback(&context);
        }
    }
}

#endif