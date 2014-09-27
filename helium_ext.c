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
    .message = strndup(message, n),
    .count = n,
    .conn = (helium_connection_t *)conn,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .cond = PTHREAD_COND_INITIALIZER,
    .next = NULL
  };

  add_queued_callback(&queued);
  
  pthread_cond_signal(&g_callback_cond);
  
  pthread_mutex_lock(&queued.mutex);
  pthread_cond_wait(&queued.cond, &queued.mutex);
  pthread_mutex_unlock(&queued.mutex);
}

static VALUE helium_rb_allocate(VALUE klass)
{
  return Data_Wrap_Struct(klass, NULL, helium_free, helium_alloc());
}

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
  RB_GC_GUARD(block);
  helium_set_context(conn, (void *)block);

  helium_open(conn, proxy_addr, helium_rb_callback);

  return self;
}

static VALUE helium_rb_subscribe(VALUE self, VALUE rb_mac, VALUE rb_token)
{
  Check_Type(rb_mac, T_FIXNUM);
  Check_Type(rb_token, T_STRING);
  helium_connection_t *conn = NULL;
  Data_Get_Struct(self, helium_connection_t, conn);

  uint64_t mac = (uint64_t)NUM2ULL(rb_mac);
  char *token_p = StringValuePtr(rb_token);

  helium_token_t token;
  memcpy(token, token_p, sizeof(helium_token_t));

  int result = helium_subscribe(conn, mac, token);
  return INT2FIX(result);
}

static VALUE helium_rb_send(VALUE self, VALUE rb_mac, VALUE rb_token, VALUE rb_message)
{
  Check_Type(rb_mac, T_FIXNUM);
  Check_Type(rb_token, T_STRING);
  Check_Type(rb_message, T_STRING);
  
  helium_connection_t *conn = NULL;
  Data_Get_Struct(self, helium_connection_t, conn);
  
  uint64_t mac = (uint64_t)NUM2ULL(rb_mac);
  char *token_p = StringValuePtr(rb_token);
  char *msg = StringValuePtr(rb_message);

  helium_token_t token;
  memcpy(token, token_p, sizeof(helium_token_t));
  
  size_t msg_len = strlen(msg);
  int result = helium_send(conn, mac, token, (unsigned char*)msg, msg_len);

  
  return INT2FIX(result);
}

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
  rb_define_method(cConnection, "close", helium_rb_close, 0);
  rb_thread_create(helium_event_thread, NULL);
}
