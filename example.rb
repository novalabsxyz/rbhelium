require 'rbhelium'
require 'base64'
require 'msgpack'

Thread.abort_on_exception = true

token = Base64.decode64("PbOkU4Jo+NObbPe27MJGNQ==")

conn = Helium::Connection.new("r01.sjc.helium.io") do |mac, datums|
  puts "Got data #{datums} from mac #{mac}"
end

puts "subscribing..."

status = conn.subscribe(0x000000fffff00002, token)

msg = ("hello from ruby #{Process.pid}").to_msgpack
conn.write(0x000000fffff00002, token, msg)

puts "status: #{status}"

sleep 15
