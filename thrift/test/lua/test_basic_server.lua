-- Copyright 2004-present Facebook. All Rights Reserved.

--------------------------------------------------------------------------------
-- Handler
TestHandler = ThriftTestIface:new{}

-- Stops the server
function TestHandler:testVoid()
  self.__server:stop()
end

function TestHandler:testString(str)
  return str .. 'bar'
end

function TestHandler:testByte(byte)
  return byte
end

function TestHandler:testI32(i32)
  return i32
end

function TestHandler:testI64(i64)
  return i64
end

function TestHandler:testDouble(d)
  return d
end

function TestHandler:testStruct(thing)
  return thing
end

--------------------------------------------------------------------------------
-- Test
local server

function luaunit:teardown()
  if server then
    server:close()
  end
end

function luaunit:testBasicServer()
  -- Handler & Processor
  local handler = TestHandler:new{}
  self:assertNotNil(handler, 'Failed to create handler')
  local processor = ThriftTestProcessor:new{
    handler = handler
  }
  self:assertNotNil(processor, 'Failed to create processor')

  -- Server Socket
  local socket = TServerSocket:new{
    port = 30118
  }
  self:assertNotNil(socket, 'Failed to create server socket')

  -- Transport & Factory
  local trans_factory = TFramedTransportFactory:new{}
  self:assertNotNil(trans_factory, 'Failed to create framed transport factory')
  local prot_factory = TBinaryProtocolFactory:new{}
  self:assertNotNil(prot_factory, 'Failed to create binary protocol factory')

  -- Simple Server
  server = TSimpleServer:new{
    processor = processor,
    serverTransport = socket,
    transportFactory = trans_factory,
    protocolFactory = prot_factory
  }
  self:assertNotNil(server, 'Failed to create server')

  -- Serve
  server:serve()
  server = nil
end
