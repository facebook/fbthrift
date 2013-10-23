-- Copyright 2004-present Facebook. All Rights Reserved.

local client

function luaunit:teardown()
  if client then
    client:close()
  end
end

function luaunit:testTSocketClient()
  -- Create
  client = TSocket:new{
    port = 30118
  }
  self:assertNotNil(client, 'Failed to create TSocket client')

  -- Set timeout
  client:setTimeout(5000)

  -- Attempt to connect
  local status, _ = pcall(client.open, client)
  self:assertTrue(status, 'Failed to connect to server')

  -- Send
  client:write('abcdef')

  -- Receive
  local got = client:read(4)
  self:assertNotNil(got, 'Failed to read 4 bytes from server TSocket')
  self:assertEqual(got, '1234', 'Incorrect data received from server TSocket')

  -- Close
  client:close()
  client = nil
end
