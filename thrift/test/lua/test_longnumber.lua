-- Copyright 2004-present Facebook. All Rights Reserved.

-- alias
local long = lualongnumber.new

-- Hex must be prefixed with '0x'
local val_plus_one = long('0x7fffffffffffffff')

-- 0x8000000000000000
local overflow = long('-9223372036854775808')

-- 0x7ffffffffffffffe
local val = long('9223372036854775806')

local one = long(1)

function luaunit:testEquals()
  self:assertEqual(val, val)

  -- Lua's default policy is to return false if the types are not equal
  self:assertNotEqual(one, 1)
  self:assertNotEqual(val_plus_one, 9223372036854775807)
end

function luaunit:testAddition()
  self:assertEqual(val + one, val_plus_one)
  self:assertEqual(val + 1, val_plus_one)
  self:assertEqual(val_plus_one + one, overflow)
  self:assertEqual(val_plus_one + 1, overflow)
end

function luaunit:testSubtraction()
  self:assertEqual(val_plus_one - one, val)
  self:assertEqual(val_plus_one - 1, val)
  self:assertEqual(overflow - one, val_plus_one)
  self:assertEqual(overflow - 1, val_plus_one)
end

function luaunit:testLessThan()
  self:assertTrue(val < val_plus_one)
  self:assertFalse(val + 1 < val_plus_one)
end

function luaunit:testLessThanEqual()
  self:assertTrue(val <= val_plus_one)
  self:assertTrue(val + 1 <= val_plus_one)
  self:assertFalse(val + 1 <= val)
end

function luaunit:testGreaterThan()
  self:assertFalse(val > val_plus_one)
  self:assertFalse(val + 1 > val_plus_one)
  self:assertTrue(val + 1 > val)
end

function luaunit:testGreaterThanEqual()
  self:assertFalse(val >= val_plus_one)
  self:assertTrue(val + 1 >= val_plus_one)
  self:assertTrue(val + 1 >= val)
end

function luaunit:testPower()
  local base = long(3037000499)
  local result = long('9223372030926248960')
  self:assertEqual(base ^ 2, result)
  self:assertEqual(long(2) ^ 2, long(4))
end

function luaunit:testDivide()
  self:assertEqual(long(30) / long(2), long(15))
  self:assertEqual(long(-30) / long(2), long(-15))
  self:assertEqual(long(-30) / long(-2), long(15))
end

function luaunit:testMultiply()
  self:assertEqual(long(30) * long(2), long(60))
  self:assertEqual(long(-30) * long(2), long(-60))
  self:assertEqual(long(-30) * long(-2), long(60))
end

function luaunit:testModulo()
  self:assertEqual(long(30) % long(2), long(0))
  self:assertEqual(long(-30) % long(2), long(0))
  self:assertEqual(long(-30) % long(-2), long(0))
  self:assertEqual(long(31) % long(3), long(1))
  self:assertEqual(long(32) % long(3), long(2))
  self:assertEqual(long(-31) % long(3), long(-1))
  self:assertEqual(long(-32) % long(3), long(-2))
  self:assertEqual(long(-31) % long(-3), long(-1))
  self:assertEqual(long(-32) % long(-3), long(-2))
end

function luaunit:testNegative()
  local positive = long(1337)
  local negative = -positive
  self:assertNotEqual(positive, negative)
  self:assertEqual(-positive, negative)
end
