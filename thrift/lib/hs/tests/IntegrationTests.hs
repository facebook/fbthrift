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

{-# LANGUAGE OverloadedStrings #-}

module IntegrationTests where

import Control.Exception
import Foreign.Ptr
import Foreign.Storable
import Test.QuickCheck

import Thrift.Protocol.Binary
import Thrift.Protocol.Compact
import Thrift.Protocol.JSON
import Thrift.Protocol.SimpleJSON
import Thrift.Transport
import Interface
import Util

import Hs_test_Types


-- | Serialize a TestStruct from C++ and deserialize in Haskell
propCToHs :: Protocol p
             => (Ptr MemoryBuffer -> p (Ptr MemoryBuffer))
             -> (Ptr MemoryBuffer -> Ptr TestStruct -> IO ())
             -> TestStruct
             -> Property
propCToHs pCons cToHS struct = ioProperty $
  bracket c_newStructPtr c_freeTestStruct $ \structPtr ->
  bracket c_openMB tClose $ \mb -> do
    poke structPtr struct
    cToHS mb structPtr
    (== struct) <$> read_TestStruct (pCons mb)

-- | Serialize a TestStruct in Haskell and deserialize in C++
propHsToC :: Protocol p
             => (Ptr MemoryBuffer -> p (Ptr MemoryBuffer))
             -> (Ptr MemoryBuffer -> IO (Ptr TestStruct))
             -> TestStruct
             -> Property
propHsToC pCons hsToC struct = ioProperty $
  bracket c_openMB tClose $ \mb -> do
    write_TestStruct (pCons mb) struct
    bracket (hsToC mb) c_freeTestStruct $ \structPtr ->
      (== struct) <$> peek structPtr

main :: IO ()
main = aggregateResults
  [ quickCheckWithResult args (propCToHs BinaryProtocol c_serializeBinary)
  , quickCheckWithResult args (propHsToC BinaryProtocol c_deserializeBinary)
  , quickCheckWithResult args (propCToHs JSONProtocol c_serializeJSON)
  , quickCheckWithResult args (propHsToC JSONProtocol c_deserializeJSON)
  , quickCheckWithResult args $
    propCToHs SimpleJSONProtocol c_serializeSimpleJSON
  , quickCheckWithResult args $
    propHsToC SimpleJSONProtocol c_deserializeSimpleJSON
  , quickCheckWithResult args (propCToHs CompactProtocol c_serializeCompact)
  , quickCheckWithResult args (propHsToC CompactProtocol c_deserializeCompact)
  ]
  where args = stdArgs
