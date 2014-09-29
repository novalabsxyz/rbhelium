rbhelium
========

Ruby bindings to libhelium.

Example
=======

```ruby
require_relative 'rbhelium'
require 'base64'

# Create a connection and associated callback function
conn = Helium::Connection.new do |mac, datums|
  puts "Got data #{datums} from mac #{mac}"
end

# Subscribe to events from a given MAC address
conn.subscribe(0x0000112233440001, "magical_helium_token")
```

Installation
============

0. You'll need libhelum, pthreads, and ruby-devel (for <ruby.h>).
1. Run `ruby extconf.rb`.
2. Run `make`.
