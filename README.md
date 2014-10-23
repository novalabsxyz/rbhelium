rbhelium
========

Ruby bindings to libhelium.

Example
=======

```ruby
require_relative 'rbhelium'
require 'base64'

token = Base64.decode64("helium_token_goes_here")

# Create a connection and associated callback function
conn = Helium::Connection.new do |mac, datums|
  puts "Got data #{datums} from mac #{mac}"
end

# Subscribe to events from a given MAC address
conn.subscribe(0x0000112233440001, token)

# Send data to a device
conn.write(0x0000112233440001, token, "here's data to send)
```

Installation
============

0. You'll need libhelium, pthreads, Bundler, and ruby-devel.
1. `git clone git@github.com:nervcorp/rbhelium ; cd rbhelium`
2. `bundle install`

If you want to install the gem globally:

3. `gem build rbhelium.gemspec`
4. `gem install rbhelium-0.0.1.gem`
