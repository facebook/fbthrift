-- Copyright 2004-present Facebook. All Rights Reserved.

local server

function luaunit:teardown()
  if server then
    server:close()
  end
end

function luaunit:testTSocketServer()
  -- Create
  server = TServerSocket:new{
    port = 30118
  }
  self:assertNotNil(server, 'Failed to create TSocket server')

  -- Listen
  server:listen()

  -- Listen for a connection
  local client = server:accept()
  self:assertNotNil(client, 'Failed to connect to client TSocket')

  -- Receive
  local got = client:read(6)
  self:assertNotNil(got, 'Failed to read 4 bytes from client TSocket')
  self:assertEqual(got, 'abcdef', 'Incorrect data received from client TSocket')

  -- Send
  client:write('1234')

  -- Close
  server:close()
  server = nil
end
