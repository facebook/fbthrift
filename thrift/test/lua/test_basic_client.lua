-- Copyright 2004-present Facebook. All Rights Reserved.

local client

function luaunit:teardown()
  if client then
    -- Shuts down the server
    client:testVoid()

    -- close the connection
    client:close()
  end
end

function luaunit:testBasicClient()
  local socket = TSocket:new{
    port = 30118
  }
  self:assertNotNil(socket, 'Failed to create client socket')
  socket:setTimeout(5000)

  local protocol = TBinaryProtocol:new{
    trans = socket
  }
  self:assertNotNil(protocol, 'Failed to create binary protocol')

  client = ThriftTestClient:new{
    protocol = protocol
  }
  self:assertNotNil(client, 'Failed to create client')

  -- Open the socket
  local status, _ = pcall(socket.open, socket)
  self:assertTrue(status, 'Failed to connect to server')

  -- String
  self:assertEqual(client:testString('lala'),  'lalabar',  'Failed testString')
  self:assertEqual(client:testString('wahoo'), 'wahoobar', 'Failed testString')

  -- Byte
  self:assertEqual(client:testByte(0x01), 1,    'Failed testByte 1')
  self:assertEqual(client:testByte(0x40), 64,   'Failed testByte 2')
  self:assertEqual(client:testByte(0x7f), 127,  'Failed testByte 3')
  self:assertEqual(client:testByte(0x80), -128, 'Failed testByte 4')
  self:assertEqual(client:testByte(0xbf), -65,  'Failed testByte 5')
  self:assertEqual(client:testByte(0xff), -1,   'Failed testByte 6')
  self:assertEqual(client:testByte(128), -128,  'Failed testByte 7')
  self:assertEqual(client:testByte(255), -1,    'Failed testByte 8')

  -- I32
  self:assertEqual(client:testI32(0x00000001), 1,           'Failed testI32 1')
  self:assertEqual(client:testI32(0x40000000), 1073741824,  'Failed testI32 2')
  self:assertEqual(client:testI32(0x7fffffff), 2147483647,  'Failed testI32 3')
  self:assertEqual(client:testI32(0x80000000), -2147483648, 'Failed testI32 4')
  self:assertEqual(client:testI32(0xbfffffff), -1073741825, 'Failed testI32 5')
  self:assertEqual(client:testI32(0xffffffff), -1,          'Failed testI32 6')
  self:assertEqual(client:testI32(2147483648), -2147483648, 'Failed testI32 7')
  self:assertEqual(client:testI32(4294967295), -1,          'Failed testI32 8')

  -- I64 (lua only supports 16 decimal precision so larger numbers are
  -- initialized by their string value)
  local long = lualongnumber.new
  self:assertEqual(client:testI64(long(0x0000000000000001)),
                   long(1),
                   'Failed testI64 1')
  self:assertEqual(client:testI64(long(0x4000000000000000)),
                   long(4611686018427387904),
                   'Failed testI64 2')
  self:assertEqual(client:testI64(long('0x7fffffffffffffff')),
                   long('9223372036854775807'),
                   'Failed testI64 3')
  self:assertEqual(client:testI64(long(0x8000000000000000)),
                   long(-9223372036854775808),
                   'Failed testI64 4')
  self:assertEqual(client:testI64(long('0xbfffffffffffffff')),
                   long('-4611686018427387905'),
                   'Failed testI64 5')
  self:assertEqual(client:testI64(long('0xffffffffffffffff')),
                   long(-1),
                   'Failed testI64 6')

  -- Double
  self:assertEqual(
      client:testDouble(1.23456789), 1.23456789, 'Failed testDouble 1')
  self:assertEqual(
      client:testDouble(0.123456789), 0.123456789, 'Failed testDouble 2')
  self:assertEqual(
      client:testDouble(0.123456789), 0.123456789, 'Failed testDouble 3')

  -- Accuracy of 16 decimal digits
  local a, b = 1.1234567890666663, 1.1234567890666661
  self:assertNotEqual(a, b)
  self:assertNotEqual(client:testDouble(a), b, 'Failed testDouble 4')

  -- Accuracy of 16 decimal digits (rounds)
  local a, b = 1.12345678906666663, 1.12345678906666661
  self:assertEqual(a, b)
  self:assertEqual(client:testDouble(a), b, 'Failed testDouble 5')

  -- Struct
  local a = {
    string_thing = 'Zero',
    byte_thing = 1,
    i32_thing = -3,
    i64_thing = long(-5)
  }
  -- TODO abirchall
  --self:assertEuqal(client:testStruct(a), a, 'Failed testStruct')

  -- Call the void function and end the test (handler stops server)
  client:testVoid()
end
