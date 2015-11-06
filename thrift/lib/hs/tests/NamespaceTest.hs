module NamespaceTest where

import Control.Applicative
import Test.QuickCheck
import Thrift.Protocol.Binary
import Thrift.Protocol.Compact
import Thrift.Protocol.JSON
import Thrift.Protocol.SimpleJSON
import Thrift.Transport
import Thrift.Transport.Empty

import Thrift.Test.Namespace_Types

import Util

prop_roundtrip :: (Transport t, Protocol p) => p t -> X -> Bool
prop_roundtrip p = liftA2 (==) id (decode_X p . encode_X p)

main :: IO ()
main = aggregateResults $ quickCheckResult <$>
       [ prop_roundtrip $ BinaryProtocol EmptyTransport
       , prop_roundtrip $ SimpleJSONProtocol EmptyTransport
       , prop_roundtrip $ JSONProtocol EmptyTransport
       , prop_roundtrip $ CompactProtocol EmptyTransport
       ]
