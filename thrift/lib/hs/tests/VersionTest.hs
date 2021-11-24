{-
  Copyright (c) Facebook, Inc. and its affiliates.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
-}

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
