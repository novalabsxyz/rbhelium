#include <stdio.h>
#include <ruby.h>
#include <helium.h>

static VALUE mHelium;
static VALUE cConnection;

void helium_rb_callback(const helium_connection_t *conn, uint64_t sender_mac, char * const message, size_t n)
{
  printf("In helium_rb_callback");
  // rb_thread_blocking_region()
}

static VALUE helium_rb_allocate(VALUE klass)
{
  return Data_Wrap_Struct(klass, NULL, helium_free, helium_alloc());
}

static VALUE helium_rb_initialize(int argc, VALUE *argv, VALUE self)
{
  VALUE proxy_addr = Qnil;
  VALUE block = Qnil;
  rb_need_block();

  rb_scan_args(argc, argv, "01&", &proxy_addr, &block);

  helium_connection_t *conn = NULL;
  Data_Get_Struct(self, helium_connection_t, conn);

  helium_init(conn, NULL, helium_rb_callback);

  return self;
}

static VALUE helium_rb_send(VALUE self, VALUE rb_mac, VALUE rb_token, VALUE rb_message)
{
  helium_connection_t *conn = NULL;
  Data_Get_Struct(self, helium_connection_t, conn);
  
  uint64_t mac = (uint64_t)NUM2ULL(rb_mac);
  char *token_p = StringValuePtr(rb_token);
  char *msg = StringValuePtr(rb_message);

  helium_token_t token;
  memcpy(token, token_p, sizeof(helium_token_t));
  
  size_t msg_len = strlen(msg);
  helium_send(conn, mac, token, (unsigned char*)msg, msg_len);
  
  return Qnil;
}

static VALUE helium_rb_close(VALUE self)
{
  helium_connection_t *conn = NULL;
  Data_Get_Struct(self, helium_connection_t, conn);
  helium_close(conn);

  return Qnil;
}

void Init_rbhelium()
{
  mHelium = rb_define_module("Helium");
  cConnection = rb_define_class_under(mHelium, "Connection", rb_cObject);
  rb_define_alloc_func(cConnection, helium_rb_allocate);
  rb_define_method(cConnection, "initialize", helium_rb_initialize, -1);
  rb_define_method(cConnection, "send", helium_rb_send, 3);
  rb_define_method(cConnection, "close", helium_rb_close, 0);
}
