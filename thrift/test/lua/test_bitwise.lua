-- Copyright 2004-present Facebook. All Rights Reserved.

local b = luabitwise

function luaunit:testAnd()
  self:assertEqual(b.band(0xffffffff, 0), 0)
  self:assertEqual(b.band(0xffffffff, 0xffffffff), -1)
  self:assertEqual(b.band(-2147418111, 4294901760), -2147418112)
end

function luaunit:testOr()
  self:assertEqual(b.bor(0xffffffff, 0), -1)
  self:assertEqual(b.bor(0, 0), 0)
end

function luaunit:testXor()
  self:assertEqual(b.bxor(0x55555555, 0xAAAAAAAA), -1)
  self:assertEqual(b.bxor(0x55555555, 0), 0x55555555)
  self:assertEqual(b.bxor(0x55555555, 0x55555555), 0)
end

function luaunit:testNot()
  self:assertEqual(b.bnot(0xAAAAAAAA), 0x55555555)
end

function luaunit:testShiftRight()
  self:assertEqual(b.shiftr(0x55555555, 16), 0x00005555)
end

function luaunit:testShiftLeft()
  self:assertEqual(b.shiftl(0x55555555, 16), 0x55550000)
end
