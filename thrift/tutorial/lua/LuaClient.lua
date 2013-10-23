#!/usr/bin/env lua
--
-- Licensed to the Apache Software Foundation (ASF) under one
-- or more contributor license agreements. See the NOTICE file
-- distributed with this work for additional information
-- regarding copyright ownership. The ASF licenses this file
-- to you under the Apache License, Version 2.0 (the
-- "License"); you may not use this file except in compliance
-- with the License. You may obtain a copy of the License at
--
--   http://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing,
-- software distributed under the License is distributed on an
-- "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
-- KIND, either express or implied. See the License for the
-- specific language governing permissions and limitations
-- under the License.
--

-- update lua paths
package.path = '../gen-lua/?.lua;' .. package.path
package.cpath = '../../lib/lua/?.so;' .. package.cpath
package.path = '../../lib/lua/?.lua;' .. package.path

-- tutorial
require 'tutorial_Calculator'

-- lua thrift
require 'TSocket'
require 'TBufferedTransport'
require 'TBinaryProtocol'

function tutorial()
  -- Make socket
  local sock = TSocket:new{
    host = 'localhost',
    port = 9090
  }

  -- Make a buffered transport from our socket
  local transport = TBufferedTransport:new{
    trans = sock
  }

  -- Wrap in a protocol
  local protocol = TBinaryProtocol:new{
    trans = transport
  }

  -- Create a client to use the protocol encoder
  local client = CalculatorClient:new{
    iprot = protocol
  }

  -- Connect!
  transport:open()
  print('Connected')

  -- Test simple function call
  client:ping()
  print('ping()')

  -- Test simple function call with return value
  local sum = client:add(1, 2)
  print('1 + 2 = ' .. sum)

  -- Test exceptions
  local status, err = pcall(divideByZero, client)
  if not status then
    print('Expected Error: ' .. err)
  else
    print('Whoa? You know how to divide by zero?')
  end

  -- Test more complicated function call with struct
  local work = Work:new{
    op = Operation.SUBTRACT,
    num1 = 15,
    num2 = 10
  }
  local diff = client:calculate(2, work)
  print('15 - 10 = ' .. diff)

  -- Check the last result
  local log = client:getStruct(2)
  print('Check log: ' .. log.value)

  -- Close!
  transport:close()
end

function divideByZero(client)
  local work = Work:new{
    op = Operation.DIVIDE,
    num1 = 1,
    num2 = 0
  }

  local quotient = client:calculate(1, work)
end

local status, err = pcall(tutorial)
if err then
  print(err)
end
