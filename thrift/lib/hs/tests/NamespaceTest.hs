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
