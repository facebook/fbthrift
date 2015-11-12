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

{-# LANGUAGE KindSignatures #-}

module Thrift.Serializable
  ( ThriftSerializable(..)
  , Serialized(..)
  , serialize
  , deserialize
  ) where

import Data.ByteString (ByteString)
import qualified Data.ByteString.Lazy as BS

import Thrift.Protocol
import Thrift.Transport (Transport)
import Thrift.Transport.Empty

newtype Serialized (p :: * -> *) = Serialized { getBS :: ByteString }

class ThriftSerializable a where
  encode :: (Protocol p, Transport t) => p t -> a -> BS.ByteString
  decode :: (Protocol p, Transport t) => p t -> BS.ByteString -> a

serialize :: (ThriftSerializable a, Protocol p) => a -> Serialized p
serialize = s . BS.toStrict . encode prot
  where (s, prot) = (Serialized, mkProtocol EmptyTransport)
          :: Protocol p => (ByteString -> Serialized p, p EmptyTransport)

deserialize :: (ThriftSerializable a, Protocol p) => Serialized p -> a
deserialize = decode prot . BS.fromStrict . get
  where (get, prot) = (getBS, mkProtocol EmptyTransport)
          :: Protocol p => (Serialized p -> ByteString, p EmptyTransport)
