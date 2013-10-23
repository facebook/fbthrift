-- Copyright 2004-present Facebook. All Rights Reserved.

local client

function luaunit:teardown()
  if client then
    client:destroy()
  end
end

function luaunit:testLuasocketClient()
  local SOCKET_SUCCESS = 1
  local err, ret

  -- Create (bind to any socket on localhost)
  client, err = luasocket.create()
  self:assertNotNil(client, 'Failed to create client socket')
  self:assertNil(err, 'Error when creating client socket')

  -- Set Timeout (5 seconds)
  ret = client:settimeout(5 * 1000)
  self:assertEqual(ret, SOCKET_SUCCESS)

  -- Attempt to connect
  local did_connect = false
  for i=1,10 do
    ret, err = client:connect('localhost', 30118)
    if ret == SOCKET_SUCCESS then
      did_connect = true
      break
    end

    -- Give the server time to come up
    os.execute('sleep 1')
  end
  self:assertTrue(did_connect, 'Failed to connect to server socket')

  -- Send
  ret = client:send(client, 'fooBar')
  self:assertEqual(ret, SOCKET_SUCCESS, 'Failed to sendto server socket')

  -- Receive
  got, err = client:receive(client, 6)
  self:assertNotNil(got, 'No data received from server socket')
  self:assertNil(err, 'Error when receiving from server socket')
  self:assertEqual(got, '012345', 'Incorrect data receieved from server socket')

  -- Destroy
  ret = client:destroy()
  self:assertEqual(ret, SOCKET_SUCCESS, 'Failed to destroy client socket')
  client = nil
end
