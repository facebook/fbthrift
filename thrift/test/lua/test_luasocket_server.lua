-- Copyright 2004-present Facebook. All Rights Reserved.

local server

function luaunit:teardown()
  if server then
    server:destroy()
  end
end

function luaunit:testLuasocketServer()
  local SOCKET_SUCCESS = 1
  local err, ret, client

  -- Create
  server, err = luasocket.create('localhost', 30118)
  self:assertNotNil(server, 'Failed to create server socket')
  self:assertNil(err, 'Error when creating server socket')

  -- Become a server socket
  ret = server:listen()
  self:assertEqual(ret, SOCKET_SUCCESS, 'Error when calling listen')

  -- Listen for a connection
  client, err = server:accept()
  self:assertNotNil(client, 'Failed to connect to client socket')
  self:assertNil(err, 'Error when connecting to client socket')

  -- Receive
  got, err = server:receive(client, 6)
  self:assertNil(err, 'Error when receiving from client socket')
  self:assertNotNil(got, 'No data received from client socket')
  self:assertEqual(got, 'fooBar', 'Incorrect data received from client socket')

  -- Send
  ret = server:send(client, '012345')
  self:assertEqual(ret, SOCKET_SUCCESS, 'Failed to send to client socket')

  -- Destroy
  ret = server:destroy()
  self:assertEqual(ret, SOCKET_SUCCESS, 'Failed to destroy server socket')
  server = nil
end
