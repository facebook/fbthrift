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
require 'TFramedTransport'
require 'TBinaryProtocol'
require 'TServer'

local CalculatorHandler = {
  log = {}
}

function CalculatorHandler:ping()
  print('ping()')
end

function CalculatorHandler:add(n1, n2)
  print('add(' .. n1 .. ', ' .. n2 .. ')')
  return n1 + n2
end

function CalculatorHandler:calculate(logid, work)
  print('calculate(' .. logid .. ', work)')

  local val = nil
  if work.op == Operation.ADD then
    val = work.num1 + work.num2
  elseif work.op == Operation.SUBTRACT then
    val = work.num1 - work.num2
  elseif work.op == Operation.MULTIPLY then
    val = work.num1 * work.num2
  elseif work.op == Operation.DIVIDE then
    if work.num2 == 0 then
      val = InvalidOperation:new{}
      val.what = work.op
      val.why = 'Cannot divide by 0'
    else
      val = work.num1 / work.num2
    end
  else
    val = InvalidOperation:new{}
    val.what = work.op
    val.why = 'Invalid Operation'
  end

  local log = SharedStruct:new{}
  log.key = logid
  log.value = val
  self.log[logid] = log

  return val
end

function CalculatorHandler:getStruct(key)
  print('getStruct(' .. key .. ')')
  return
    self.log[key] or
    SharedStruct:new{
      ['key'] = key,
      ['value'] = 'nil'
    }
end

function CalculatorHandler:zip()
  print('zip()');
end

function CalculatorHandler:preServe(info)
  print(thrift_r(info))
end

local pro = CalculatorProcessor:new{
  handler = CalculatorHandler
}
local sock = TServerSocket:new{
  host = 'localhost',
  port = 9090
}
local tfactory = TFramedTransportFactory:new{}
local pfactory = TBinaryProtocolFactory:new{}
local server = TSimpleServer:new{
  processor = pro,
  serverTransport = sock,
  transportFactory = tfactory,
  protocolFactory = pfactory
}
server:setServerEventHandler(CalculatorHandler)

print 'Starting server...'
server:serve()
print 'done.'
