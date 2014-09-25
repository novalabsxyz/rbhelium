// Copyright (c) 2014 Helium Systems, Inc.

#include <pthread.h>
#include <stdint.h>

struct helium_queued_callback {
  uint64_t sender_mac;
  char *message;
  size_t count;

  // TODO: pthread mutexen are notoriously inefficient
  // let's look and see if libuv's are any faster. 
  pthread_mutex_t mutex;
  pthread_cond_t cond;

  struct helium_queued_callback *next;
};

struct helium_waiter_t {
  struct helium_queued_callback *callback;
  _Bool abort;
};
