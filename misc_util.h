#ifndef _MISC_UTIL_H
#define _MISC_UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "mem_util.h"

void *ptr_or_exit(void *ptr, const char *message)
{
    if (ptr == NULL)
    {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }

    return ptr;
}

int fd_or_exit(int fd, const char *message)
{
    if (fd < 0)
    {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
    return fd;
}

struct Array
{
    void **data;
    size_t length;
    size_t capacity;
};

typedef struct Array Array;

void array_init(Array *array, size_t capacity)
{
    array->data = new_array(void *, capacity);
    array->length = 0;
    array->capacity = capacity;
}

void array_resize(Array *array, size_t new_capacity)
{
    array->data = resize_array(void *, new_capacity, array->data);
    array->capacity = new_capacity;
}

void array_push(Array *array, void *value)
{
    if (array->length == array->capacity)
    {
        array_resize(array, array->capacity * 2);
    }

    array->data[array->length++] = value;
}

void *array_get(Array *array, size_t index)
{
    return array->data[index];
}

void array_free(Array *array)
{
    drop(array->data);
    drop(array);
}

void array_remove(Array *array, size_t index)
{
    if (index >= array->length)
    {
        return;
    }

    for (size_t i = index; i < array->length - 1; i++)
    {
        array->data[i] = array->data[i + 1];
    }

    array->length--;
}

struct Buffer
{
    char *data;
    size_t length;
    size_t capacity;
};

typedef struct Buffer Buffer;

void buffer_init(Buffer *buffer, size_t capacity)
{
    buffer->data = new_array(char, capacity);
    buffer->capacity = capacity;
    buffer->length = 0;
}

void buffer_resize(Buffer *buffer, size_t new_capacity)
{
    buffer->data = resize_array(char, new_capacity, buffer->data);
    buffer->capacity = new_capacity;
}

Buffer buffer_build(size_t capacity)
{
    Buffer buffer;
    buffer_init(&buffer, capacity);
    return buffer;
}

Buffer *buffer_new(size_t capacity)
{
    Buffer *buffer = new (Buffer);
    buffer_init(buffer, capacity);
    return buffer;
}

#define buffer_free(buffer) drop(buffer.data)

Buffer buffer_copy(Buffer *buffer)
{
    Buffer copy = buffer_build(buffer->capacity);
    memcpy(copy.data, buffer->data, buffer->length);
    copy.length = buffer->length;
    return copy;
}

Buffer buffer_from_string(const char *string)
{
    Buffer buffer;
    buffer.length = strlen(string) + 1;
    buffer.data = new_array(char, buffer.length);
    buffer.capacity = buffer.length;
    strcpy(buffer.data, string);
    return buffer;
}

void buffer_print(Buffer *buffer)
{
    for (size_t i = 0; i < buffer->length; i++)
    {
        char c = buffer->data[i];
        if (isprint(c))
            printf("%c", c);
        else
            printf("[%x]", c);
    }
    printf("\n");
}

void buffer_append_string(Buffer *buffer, const char *string)
{
    size_t length = strlen(string);
    size_t new_length = buffer->length + length;
    if (new_length > buffer->capacity)
    {
        buffer_resize(buffer, new_length * 2);
    }

    strcpy(buffer->data + buffer->length - 1, string);
    buffer->length = new_length;
}

void buffer_append(Buffer *buffer, Buffer *other)
{
    size_t new_length = buffer->length + other->length;
    if (new_length > buffer->capacity)
    {
        buffer_resize(buffer, new_length * 2);
    }

    memcpy(buffer->data + buffer->length, other->data, other->length);
    buffer->length = new_length;
}

#endif