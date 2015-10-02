module VersionTest where

import Data.Int
import Data.Vector
import Test.QuickCheck
import Thrift.Protocol.Binary
import Thrift.Protocol.Compact
import Thrift.Protocol.JSON
import Thrift.Protocol.SimpleJSON
import Thrift.Transport
import Thrift.Transport.Empty

import Version_Types

import Util

prop_roundtrip :: (Transport t, Protocol p) => p t -> Int32 -> Vector (Vector Int32) -> Bool
prop_roundtrip p x y = Foo1 x == decode_Foo1 p (encode_Foo2 p $ Foo2 x y)

main :: IO ()
main = aggregateResults $ quickCheckResult <$>
       [ prop_roundtrip $ BinaryProtocol EmptyTransport
       , prop_roundtrip $ SimpleJSONProtocol EmptyTransport
       , prop_roundtrip $ JSONProtocol EmptyTransport
       , prop_roundtrip $ CompactProtocol EmptyTransport
       ]
