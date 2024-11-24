/**
 * DERIVED FROM IO URING MAN PAGES EXAMPLE
 */

#ifndef _URING_CORE_H
#define _URING_CORE_H

#include <fcntl.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <unistd.h>
#include <linux/io_uring.h>
#include <stdbool.h>

#define io_uring_smp_store_release(p, v) \
  atomic_store_explicit((_Atomic typeof(*(p)) *)(p), (v), memory_order_release)
#define io_uring_smp_load_acquire(p) \
  atomic_load_explicit((_Atomic typeof(*(p)) *)(p), memory_order_acquire)

int io_uring_setup(unsigned entries, struct io_uring_params *p)
{
  return (int)syscall(__NR_io_uring_setup, entries, p);
}

int io_uring_enter(int ring_fd, unsigned int to_submit,
                   unsigned int min_complete, unsigned int flags)
{
  return (int)syscall(__NR_io_uring_enter, ring_fd, to_submit, min_complete,
                      flags, NULL, 0);
}

typedef struct io_uring_sqe UringTask;
typedef struct io_uring_cqe UringCompletion;

struct Uring
{
  int ring_fd;
  unsigned int *sring_head, *sring_tail;
  unsigned int *sring_mask, *sring_array;
  unsigned int *cring_head, *cring_tail,
      *cring_mask;
  UringTask *tasks;
  UringCompletion *completions;
  bool running;
};

typedef struct Uring Uring;

bool uring_init(Uring *uring, unsigned int queue_length)
{
  struct io_uring_params p;
  memset(&p, 0, sizeof(p));

  void *sq_ptr, *cq_ptr;

  uring->ring_fd = io_uring_setup(queue_length, &p);
  if (uring->ring_fd < 0)
  {
    perror("io_uring_setup");
    return false;
  }

  int sr_size = p.sq_off.array + p.sq_entries * sizeof(unsigned);
  int cr_size = p.cq_off.cqes + p.cq_entries * sizeof(UringCompletion);

  if (p.features & IORING_FEAT_SINGLE_MMAP)
  {
    if (cr_size > sr_size)
      sr_size = cr_size;
    cr_size = sr_size;
  }

  sq_ptr = mmap(0, sr_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE,
                uring->ring_fd, IORING_OFF_SQ_RING);

  if (sq_ptr == MAP_FAILED)
  {
    perror("mmap");
    return false;
  }

  if (p.features & IORING_FEAT_SINGLE_MMAP)
  {
    cq_ptr = sq_ptr;
  }
  else
  {
    cq_ptr = mmap(0, cr_size, PROT_READ | PROT_WRITE,
                  MAP_SHARED | MAP_POPULATE, uring->ring_fd, IORING_OFF_CQ_RING);
    if (cq_ptr == MAP_FAILED)
    {
      perror("mmap");
      return false;
    }
  }

  uring->sring_head = sq_ptr + p.sq_off.head;
  uring->sring_tail = sq_ptr + p.sq_off.tail;
  uring->sring_mask = sq_ptr + p.sq_off.ring_mask;
  uring->sring_array = sq_ptr + p.sq_off.array;

  /* Map in the submission queue entries array */
  uring->tasks = mmap(0, p.sq_entries * sizeof(UringTask),
                      PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, uring->ring_fd,
                      IORING_OFF_SQES);
  if (uring->tasks == MAP_FAILED)
  {
    perror("mmap");
    return false;
  }

  uring->cring_head = cq_ptr + p.cq_off.head;
  uring->cring_tail = cq_ptr + p.cq_off.tail;
  uring->cring_mask = cq_ptr + p.cq_off.ring_mask;
  uring->completions = cq_ptr + p.cq_off.cqes;
  uring->running = false;

  return true;
}

UringCompletion *uring_completion(Uring *uring)
{
  UringCompletion *completion;
  unsigned int head = io_uring_smp_load_acquire(uring->cring_head);

  if (head == *uring->cring_tail)
    return NULL;

  completion = &uring->completions[head & (*uring->cring_mask)];

  head++;

  io_uring_smp_store_release(uring->cring_head, head);

  return completion;
}

UringTask *uring_task(Uring *uring)
{
  unsigned int tail = *uring->sring_tail;
  unsigned int index = tail & *uring->sring_mask;
  UringTask *entry = &uring->tasks[index];
  return entry;
}

void uring_submit(Uring *uring)
{
  unsigned int tail = *uring->sring_tail;
  unsigned int index = tail & *uring->sring_mask;
  uring->sring_array[index] = index;
  tail++;
  io_uring_smp_store_release(uring->sring_tail, tail);
}

bool uring_empty(Uring *uring)
{
  return *uring->sring_tail == *uring->sring_head &&
         *uring->cring_tail == *uring->cring_head;
}

int uring_enter(Uring *uring)
{
  int size = *uring->sring_tail - *uring->sring_head;
  int ret = io_uring_enter(uring->ring_fd, size, 1, IORING_ENTER_GETEVENTS);
  if (ret < 0)
  {
    perror("io_uring_enter");
    return -1;
  }

  return ret;
}

#endif