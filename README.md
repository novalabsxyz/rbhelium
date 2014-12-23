rbhelium
========

Ruby bindings to libhelium.

Example
=======

```ruby
require 'rbhelium'
require 'base64'

# A sample Helium token for one of our public devices
token = Base64.decode64("kdTl6U1w+sR61NBiQjm8sw==")

# Create a connection and associated callback function
conn = Helium::Connection.new("r01.sjc.helium.io") do |mac, datums|
  puts "Got data #{datums} from mac #{mac}"
end

# Subscribe to events from a given MAC address, in this case our public device
conn.subscribe(0x000000fffff00001, token)

# Send data to the device
conn.write(0x000000fffff00001, token, "here's data to send")
```

Installation
============

rbhelium requires Ruby >= 2.0. You'll also need [libhelium](https://github.com/helium/libhelium), pthreads, Bundler, and ruby-devel.

1. `git clone https://github.com/helium/rbhelium.git ; cd rbhelium`
2. `bundle install`

If you want to install the gem globally:

3. `gem build rbhelium.gemspec`
4. `gem install rbhelium-0.0.1.gem`

Installation of Released rbhelium Gems
---------------------------------------

We host our gems on [Packagecloud](https://packagecloud.io/helium/libhelium). To install those gems, you can see the Gem section [of the installation page](https://packagecloud.io/helium/libhelium/install).


Troubleshooting
---------------

If you don't have libhelium installed, you will get this error when building/installing the gem:

```
[ERROR]: missing `helium`. Aborting.
*** extconf.rb failed ***
Could not create Makefile due to some reason, probably lack of necessary
libraries and/or headers.  Check the mkmf.log file for more details.  You may
need configuration options.
```

Be aware that the callback block runs in a different thread, so for debugging you should set Thread.abort_on_exception=true
