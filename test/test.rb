require_relative '../rbhelium'
require 'base64'
require 'minitest/unit'
require 'minitest/autorun'

TEST_MAC = 0x0000112233440001
TOKEN = Base64.decode64("C8Slmiwm6dreZrUhy5YPiA==")

class TestHeliumSubscriptions < MiniTest::Unit::TestCase
  def setup()
    @succeeded = false
    @connection = Helium::Connection.new() do |mac, datums|
      @succeeded = true
    end
  end

  def test_add_subscription
    status = @connection.subscribe(TEST_MAC, TOKEN)
    assert_equal(status, 0)
    sleep(5)
    assert(@succeeded, "Callback block did not execute")
  end
end

class TestAPI < MiniTest::Unit::TestCase
  def test_type_errors()
    assert_raises(LocalJumpError) do
      Helium::Connection.new
    end
    
    assert_raises(TypeError) do
      Helium::Connection.new(-100000) {}
    end

    conn = Helium::Connection.new {}

    assert_raises(TypeError) do
      conn.subscribe("this shouldn't", "work")
    end
  end
end
