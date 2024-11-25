#ifndef _MEM_UTIL_H
#define _MEM_UTIL_H

#include <stdlib.h>
#include <stdio.h>

#define TO_STRING_INNER(x) #x
#define TO_STRING(x) TO_STRING_INNER(x)

// #define new(T) ((T *)malloc(sizeof(T)))
// #define new_array(T, N) ((T *)malloc(sizeof(T) * N))
// #define drop(ptr) free(ptr)

#define new(T) ((T *)allocate(sizeof(T), #T " at " __FILE__ ":" TO_STRING(__LINE__)))
#define new_array(T, N) ((T *)allocate(sizeof(T) * N, #T "[" #N "] at " __FILE__ ":" TO_STRING(__LINE__)))
#define resize_array(T, N, ptr) ((T *)reallocate(ptr, sizeof(T) * N, #T "[" #N "] at " __FILE__ ":" TO_STRING(__LINE__)))
#define drop(ptr) deallocate(ptr, __FILE__ ":" TO_STRING(__LINE__))

struct Allocation
{
    void *ptr;
    const char *info;
};

typedef struct Allocation Allocation;

Allocation *allocations = NULL;
int allocation_length = 0;
int allocation_capacity = 0;

void allocation_push(void *ptr, const char *info)
{
    Allocation allocation = {ptr, info};

    if (allocations == NULL)
    {
        allocation_capacity = 4;
        allocations = malloc(sizeof(Allocation) * allocation_capacity);
        allocation_length = 0;
    }

    for (int i = 0; i < allocation_length; i++)
    {
        if (allocations[i].ptr != NULL)
            continue;

        allocations[i] = allocation;
        return;
    }

    if (allocation_length == allocation_capacity)
    {
        allocation_capacity *= 2;
        allocations = realloc(allocations, sizeof(Allocation) * allocation_capacity);
    }

    allocations[allocation_length++] = allocation;
}

void allocation_summary()
{
    printf("Memory allocations:\n");

    for (int i = 0; i < allocation_length; i++)
    {
        Allocation allocation = allocations[i];
        if (allocation.ptr == NULL)
            continue;

        printf("- %s\n", allocation.info);
    }
}

void *reallocate(void *ptr, size_t size, const char *info)
{
    void *new_ptr = realloc(ptr, size);
    if (new_ptr == NULL)
    {
        perror("realloc");
        exit(1);
    }

    for (int i = 0; i < allocation_length; i++)
    {
        if (allocations[i].ptr == ptr)
        {
            allocations[i].ptr = new_ptr;
            allocations[i].info = info;
            return new_ptr;
        }
    }

    fprintf(stderr, "Attempted to reallocate unallocated memory at %s\n", info);
    return new_ptr;
}

void *allocate(size_t size, const char *info)
{
    void *ptr = malloc(size);
    if (ptr == NULL)
    {
        perror("malloc");
        exit(1);
    }

    allocation_push(ptr, info);

    return ptr;
}

void deallocate(void *ptr, const char *info)
{

    for (int i = 0; i < allocation_length; i++)
    {
        if (allocations[i].ptr == ptr)
        {
            allocations[i].ptr = NULL;
            free(ptr);
            return;
        }
    }

    fprintf(stderr, "Attempted to deallocate unallocated memory at %s\n", info);
}

#endif