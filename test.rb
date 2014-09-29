require_relative 'rbhelium'
require 'base64'
require 'minitest/unit'
require 'minitest/autorun'

TEST_MAC = 0x0000112233440001
TOKEN = Base64.decode64("C8Slmiwm6dreZrUhy5YPiA==")

class TestHeliumSubscriptions < MiniTest::Unit::TestCase
  def setup()
    @succeeded = false
    @connection = Helium::Connection.new do |mac, datums|
      @succeeded = true
    end
  end

  def test_add_subscription
    status = @connection.subscribe(TEST_MAC, TOKEN)
    assert_equal(status, 0)
    sleep(5)
    assert(@succeeded)
  end
end
