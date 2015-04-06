/*
 * Ruby API for the Helium platform.
 *
 * Copyright (C) 2014 Helium Systems Inc.
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <stdio.h>
#include <ruby.h>
#include <ruby/thread.h>
#include <helium.h>
#include "helium_ext.h"
#include <assert.h>


static VALUE mHelium;
static VALUE cConnection;

static pthread_mutex_t g_callback_queue_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_callback_cond = PTHREAD_COND_INITIALIZER;
static struct helium_queued_callback *g_queued_callbacks = NULL;

void add_queued_callback(struct helium_queued_callback *new_callback)
{
  pthread_mutex_lock(&g_callback_queue_lock);
  new_callback->next = g_queued_callbacks;
  g_queued_callbacks = new_callback;
  pthread_mutex_unlock(&g_callback_queue_lock);
}

struct helium_queued_callback *pop_queued_callback()
{
  struct helium_queued_callback *to_return = g_queued_callbacks;
  if (g_queued_callbacks) {
    g_queued_callbacks = g_queued_callbacks->next;
  }
  return to_return;
}

void helium_rb_callback(const helium_connection_t *conn, uint64_t sender_mac, char * const message, size_t n)
{
  struct helium_queued_callback queued = {
    .sender_mac = sender_mac,
    .message = malloc(n),
    .count = n,
    .conn = (helium_connection_t *)conn,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .cond = PTHREAD_COND_INITIALIZER,
    .next = NULL
  };

  memcpy(queued.message, message, n);

  add_queued_callback(&queued);

  pthread_cond_signal(&g_callback_cond);

  pthread_mutex_lock(&queued.mutex);
  pthread_cond_wait(&queued.cond, &queued.mutex);
  pthread_mutex_unlock(&queued.mutex);
}

void helium_rb_mark(void * p) {
  helium_connection_t *conn = (helium_connection_t*)p;
  // make sure we mark the callback proc too so it doesn't get GCed
  rb_gc_mark(helium_get_context(conn));
}

static VALUE helium_rb_allocate(VALUE klass)
{
  return Data_Wrap_Struct(klass, helium_rb_mark, helium_free, helium_alloc());
}

/*
 * call-seq:
 *    Helium::Connection.new(proxy=nil) { |mac, str| ... } => Helium::Connection
 *
 * Open a Helium connection, receiving data with the provided block.
 */
static VALUE helium_rb_initialize(int argc, VALUE *argv, VALUE self)
{
  VALUE rb_proxy_addr = Qnil;
  VALUE block = Qnil;
  rb_need_block();

  rb_scan_args(argc, argv, "01&", &rb_proxy_addr, &block);

  char *proxy_addr = NULL;
  if (!NIL_P(rb_proxy_addr)) {
    proxy_addr = StringValuePtr(rb_proxy_addr);
  }

  helium_connection_t *conn = NULL;
  Data_Get_Struct(self, helium_connection_t, conn);
  helium_set_context(conn, (void *)block);
  RB_GC_GUARD(block);

  helium_open(conn, proxy_addr, helium_rb_callback);

  return self;
}

/*
 * call-seq:
 *    conn.subscribe(mac, token)            => Fixnum
 *
 * Subscribe to messages from a given device specified by the MAC address (as a Fixnum)
 * and verified with the provided token (a String)
 *
 * Returns an error code, or 0 on success.
 */
static VALUE helium_rb_subscribe(VALUE self, VALUE rb_mac, VALUE rb_token)
{
  if (TYPE(rb_mac) != T_FIXNUM && TYPE(rb_mac) != T_BIGNUM) {
    rb_raise(rb_eTypeError, "expected FixNum or Bignum");
  }
  Check_Type(rb_token, T_STRING);
  helium_connection_t *conn = NULL;
  Data_Get_Struct(self, helium_connection_t, conn);

  uint64_t mac = (uint64_t)NUM2ULL(rb_mac);
  char *token_p = RSTRING_PTR(rb_token);

  helium_token_t token;
  memcpy(token, token_p, sizeof(helium_token_t));

  int result = helium_subscribe(conn, mac, token);
  return INT2FIX(result);
}

/*
 * call-seq:
 *    conn.unsubscribe(mac)            => Fixnum
 *
 * Unsubscribes from the device specified by the provided mac.
 *
 * Returns an error code, or 0 on success.
 */
static VALUE helium_rb_unsubscribe(VALUE self, VALUE rb_mac)
{
  if (TYPE(rb_mac) != T_FIXNUM && TYPE(rb_mac) != T_BIGNUM) {
    rb_raise(rb_eTypeError, "expected FixNum or Bignum");
  }
  helium_connection_t *conn = NULL;
  Data_Get_Struct(self, helium_connection_t, conn);

  uint64_t mac = (uint64_t)NUM2ULL(rb_mac);

  int result = helium_unsubscribe(conn, mac);
  return INT2FIX(result);
}

/*
 * call-seq:
 *    conn.send(mac, token, message)            => Fixnum
 *
 * Sends the provided +message+ to the Helium device specified by +mac+.
 *
 * Returns an error code, or 0 on success.
 */
static VALUE helium_rb_send(VALUE self, VALUE rb_mac, VALUE rb_token, VALUE rb_message)
{
  if (TYPE(rb_mac) != T_FIXNUM && TYPE(rb_mac) != T_BIGNUM) {
    rb_raise(rb_eTypeError, "expected FixNum or Bignum");
  }
  Check_Type(rb_token, T_STRING);
  Check_Type(rb_message, T_STRING);

  helium_connection_t *conn = NULL;
  Data_Get_Struct(self, helium_connection_t, conn);

  uint64_t mac = (uint64_t)NUM2ULL(rb_mac);
  char *token_p = RSTRING_PTR(rb_token);
  char *msg = RSTRING_PTR(rb_message);

  helium_token_t token;
  memcpy(token, token_p, sizeof(helium_token_t));

  size_t msg_len = RSTRING_LEN(rb_message);
  int result = helium_send(conn, mac, token, (unsigned char*)msg, msg_len);


  return INT2FIX(result);
}

/*
 * call-seq:
 *    conn.close()                 => nil
 *
 * Closes the connection.
 */
static VALUE helium_rb_close(VALUE self)
{
  helium_connection_t *conn = NULL;
  Data_Get_Struct(self, helium_connection_t, conn);
  helium_close(conn);

  return Qnil;
}

static VALUE helium_callback_handler_thread(void *ctx)
{
  struct helium_queued_callback *callback = (struct helium_queued_callback *)ctx;

  VALUE rb_mac = ULL2NUM((unsigned long long)callback->sender_mac);
  VALUE rb_message = rb_str_new(callback->message, callback->count);

  VALUE proc = (VALUE)helium_get_context(callback->conn);
  VALUE result = rb_funcall(proc, rb_intern("call"), 2, rb_mac, rb_message);

  pthread_mutex_lock(&callback->mutex);
  pthread_cond_signal(&callback->cond);
  pthread_mutex_unlock(&callback->mutex);

  return result;
}

static void *wait_for_callback(void *ctx)
{
  struct helium_waiter_t *waiter = (struct helium_waiter_t *)ctx;

  pthread_mutex_lock(&g_callback_queue_lock);

  while((waiter->abort == 0) &&
        (waiter->callback = pop_queued_callback()) == NULL) {
    // TODO: that condition sucks
    pthread_cond_wait(&g_callback_cond, &g_callback_queue_lock);
  }

  pthread_mutex_unlock(&g_callback_queue_lock);
  return NULL;
}

static void stop_waiting(void *ctx)
{
  struct helium_waiter_t *waiter = (struct helium_waiter_t *)ctx;
  pthread_mutex_lock(&g_callback_queue_lock);
  waiter->abort = 1;
  pthread_cond_signal(&g_callback_cond);
  pthread_mutex_unlock(&g_callback_queue_lock);
}

static VALUE helium_event_thread(void *unused)
{
  struct helium_waiter_t waiter = {
    .callback = NULL,
    .abort = 0
  };

  while (waiter.abort == 0) {
    rb_thread_call_without_gvl(wait_for_callback, &waiter, &stop_waiting, &waiter);

    if (waiter.callback != NULL) {
      rb_thread_create(helium_callback_handler_thread, (void *)waiter.callback);
    }
  }

  return Qnil;
}

void Init_rbhelium()
{
  mHelium = rb_define_module("Helium");
  cConnection = rb_define_class_under(mHelium, "Connection", rb_cObject);
  rb_define_alloc_func(cConnection, helium_rb_allocate);
  rb_define_method(cConnection, "initialize", helium_rb_initialize, -1);
  rb_define_method(cConnection, "write", helium_rb_send, 3);
  rb_define_method(cConnection, "subscribe", helium_rb_subscribe, 2);
  rb_define_method(cConnection, "unsubscribe", helium_rb_unsubscribe, 1);
  rb_define_method(cConnection, "close", helium_rb_close, 0);
  rb_thread_create(helium_event_thread, NULL);
}
