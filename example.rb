require_relative 'rbhelium'
require 'base64'

token = Base64.decode64("C8Slmiwm6dreZrUhy5YPiA==")

conn = Helium::Connection.new do |mac, datums|
  puts "Got data #{datums} from mac #{mac}"
end

puts "subscribing..."

status = conn.subscribe(0x0000112233440001, token)

puts "status: #{status}"

sleep 15
