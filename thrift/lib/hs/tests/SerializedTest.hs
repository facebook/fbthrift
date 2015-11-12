module SerializedTest where

import Test.QuickCheck

import Thrift.Serializable
import Thrift.Protocol.JSON
import Util
import Hs_test_Types

propRoundTripSerializable :: TestStruct -> Bool
propRoundTripSerializable struct =
  let bs = serialize struct :: Serialized JSONProtocol
  in struct == deserialize bs

main :: IO ()
main = aggregateResults
  [ quickCheckResult propRoundTripSerializable
  ]
