require_relative 'rbhelium'
require 'base64'

# the access token for a device, you'll have to get this out of fusion or something
token = Base64.decode64("C8Slmiwm6dreZrUhy5YPiA==")
# the MAC address of a device
mac = 0x0000112233440001

# you always need a connection, the callback is where received events end up
conn = Helium::Connection.new do |mac, packet|
  puts "Got data #{mac}, #{packet}"
end

puts "subscribing..."

# add a device to the connection's subscription list
status = conn.subscribe(mac, token)

puts "status: #{status}"

# send the device a message
status = conn.send(mac, token, "hello, from ruby land")

puts "status: #{status}"

sleep 15
