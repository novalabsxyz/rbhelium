require_relative 'rbhelium'
require 'base64'

token = Base64.decode64("C8Slmiwm6dreZrUhy5YPiA==")

conn = Helium::Connection.new do |datums|
  puts "Got data #{datums}"
end

result = conn.send(0x0000112233440001, token, "s")
puts "send result is #{result}"

sleep 15
