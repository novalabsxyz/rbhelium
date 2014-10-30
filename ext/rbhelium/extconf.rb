# coding: utf-8
require 'mkmf'

def must_have(type, ident)
  abort "[ERROR]: missing `#{ident}`. Aborting." unless send("have_#{type}", ident)
end

must_have :header, 'ruby.h'
must_have :func, 'rb_thread_blocking_region'
must_have :library, 'pthread'
must_have :library, 'helium'
must_have :library, 'crypto'
must_have :library, 'uv'

$CFLAGS << ' -O0 -ggdb -Wall'

create_makefile 'rbhelium/rbhelium'
