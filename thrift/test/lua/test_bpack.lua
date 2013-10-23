-- Copyright 2004-present Facebook. All Rights Reserved.

local b = luabpack

-- Pack and unpack a value
function pup(t, val)
  return luabpack.bunpack(t, luabpack.bpack(t, val))
end

function luaunit:testChar()
  luaunit:assertEqual(pup('c', 0x01), 1)
  luaunit:assertEqual(pup('c', 0x40), 64)
  luaunit:assertEqual(pup('c', 0x7f), 127)
  luaunit:assertEqual(pup('c', 0x80), -128)
  luaunit:assertEqual(pup('c', 0xbf), -65)
  luaunit:assertEqual(pup('c', 0xff), -1)

  luaunit:assertEqual(pup('c', 128), -128) -- negative overflow
  luaunit:assertEqual(pup('c', 255), -1)
end

function luaunit:testShort()
  luaunit:assertEqual(pup('s', 0x0001), 1)
  luaunit:assertEqual(pup('s', 0x4000), 16384)
  luaunit:assertEqual(pup('s', 0x7fff), 32767)
  luaunit:assertEqual(pup('s', 0x8000), -32768)
  luaunit:assertEqual(pup('s', 0xbfff), -16385)
  luaunit:assertEqual(pup('s', 0xffff), -1)

  luaunit:assertEqual(pup('s', 32768), -32768) -- negative overflow
  luaunit:assertEqual(pup('s', 65535), -1)
end

function luaunit:testInt()
  luaunit:assertEqual(pup('i', 0x00000001), 1)
  luaunit:assertEqual(pup('i', 0x40000000), 1073741824)
  luaunit:assertEqual(pup('i', 0x7fffffff), 2147483647)
  luaunit:assertEqual(pup('i', 0x80000000), -2147483648)
  luaunit:assertEqual(pup('i', 0xbfffffff), -1073741825)
  luaunit:assertEqual(pup('i', 0xffffffff), -1)

  luaunit:assertEqual(pup('i', 2147483648), -2147483648)  -- negative overflow
  luaunit:assertEqual(pup('i', 4294967295), -1)
end

function luaunit:testLong()
  local long = lualongnumber.new
  luaunit:assertEqual(pup('l', long(0x0000000000000001)), long(1))
  luaunit:assertEqual(
    pup('l', long(0x4000000000000000)), long(4611686018427387904))

  -- 0x7fffffffffffffff
  luaunit:assertEqual(
    pup('l', long('9223372036854775807')), long('9223372036854775807'))

  -- 0x8000000000000000
  luaunit:assertEqual(
    pup('l', long(-9223372036854775808)), long(-9223372036854775808))

  -- 0xbfffffffffffffff
  luaunit:assertEqual(
    pup('l', '-4611686018427387903'), long("-4611686018427387903"))

  -- 0xffffffffffffffff
  luaunit:assertEqual(pup('l', long(-1)), long(-1))
end

function luaunit:testDouble()
  luaunit:assertEqual(pup('d', 1.23456789), 1.23456789)
  luaunit:assertEqual(pup('d', 0.123456789), 0.123456789)

  local a,b = 1.1234567890666663,
              1.1234567890666661 -- Accuracy of 16 decimal digits
  luaunit:assertNotEqual(a, b)
  luaunit:assertNotEqual(pup('d', a), b)

  local a,b = 1.12345678906666663,
              1.12345678906666661 -- Accuracy of 16 decimal digits (rounds)
  luaunit:assertEqual(a, b)
  luaunit:assertEqual(pup('d', a), b)
end
