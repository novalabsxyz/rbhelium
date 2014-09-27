require_relative 'rbhelium'
require 'base64'

token = Base64.decode64("C8Slmiwm6dreZrUhy5YPiA==")

conn = Helium::Connection.new do |mac, packet|
  puts "Got data #{mac}, #{packet}"
end

puts "subscribing..."

status = conn.subscribe(0x0000112233440001, token)

puts "status: #{status}"

status = conn.send(0x0000112233440001, token, "hello, from ruby land")

puts "status: #{status}"

sleep 15
