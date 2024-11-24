#ifndef _MISC_UTIL_H
#define _MISC_UTIL_H

#include <stdio.h>
#include <stdlib.h>

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

Array *array_new(size_t capacity)
{
    Array *array = malloc(sizeof(Array));
    array->data = malloc(capacity * sizeof(void *));
    array->length = 0;
    array->capacity = capacity;
    return array;
}

void array_resize(Array *array, size_t new_capacity)
{
    array->data = realloc(array->data, new_capacity * sizeof(void *));
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
    free(array->data);
    free(array);
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

#endif