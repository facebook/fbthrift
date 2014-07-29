{-# LANGUAGE CPP #-}
{-# LANGUAGE DeriveDataTypeable #-}
{-# LANGUAGE OverloadedStrings #-}
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

module Thrift.Protocol
    ( Protocol(..)
    , ProtocolExn(..)
    , ProtocolExnType(..)
    , runParser
    , versionMask
    , version1
    , bsToFloating
    ) where

import Control.Exception
import Data.Attoparsec.ByteString
import Data.ByteString.Lazy (ByteString, toStrict)
import Data.ByteString.Unsafe
import Data.Functor ((<$>))
import Data.Int
import Data.Monoid (mempty)
import Data.Text.Lazy (Text)
import Data.Typeable (Typeable)
import Data.Word
import Foreign.Ptr (castPtr)
import Foreign.Storable (Storable, peek, poke)
import System.IO.Unsafe
import qualified Data.ByteString as BS

import Thrift.Types
import Thrift.Transport

versionMask :: Int32
versionMask = fromIntegral (0xffff0000 :: Word32)

version1 :: Int32
version1 = fromIntegral (0x80010000 :: Word32)

class Protocol a where
  getTransport :: Transport t => a t -> t

  writeMessage :: Transport t => a t -> (Text, MessageType, Int32) -> IO ()
  readMessage :: Transport t => a t -> IO (Text, MessageType, Int32)

  serializeVal :: Transport t => a t -> ThriftVal -> ByteString
  deserializeVal :: Transport t => a t -> ThriftType -> ByteString -> ThriftVal

  writeVal :: Transport t => a t -> ThriftVal -> IO ()
  writeVal p = tWrite (getTransport p) . serializeVal p
  readVal :: Transport t => a t -> ThriftType -> IO ThriftVal

data ProtocolExnType
    = PE_UNKNOWN
    | PE_INVALID_DATA
    | PE_NEGATIVE_SIZE
    | PE_SIZE_LIMIT
    | PE_BAD_VERSION
    | PE_NOT_IMPLEMENTED
    | PE_MISSING_REQUIRED_FIELD
      deriving ( Eq, Show, Typeable )

data ProtocolExn = ProtocolExn ProtocolExnType String
  deriving ( Show, Typeable )
instance Exception ProtocolExn

runParser :: (Protocol p, Transport t, Show a) => p t -> Parser a -> IO a
runParser prot p = refill >>= getResult . parse p
  where
    refill = toStrict <$> tRead (getTransport prot) 128 `catch` handleEOF
    getResult (Done _ a) = return a
    getResult (Partial k) = refill >>= getResult . k
    getResult f = error $ show f

handleEOF :: SomeException -> IO ByteString
handleEOF = const $ return mempty

-- | Converts a ByteString to a Floating point number
-- The ByteString is assumed to be encoded in network order (Big Endian)
-- therefore the behavior of this function varies based on whether the local
-- machine is big endian or little endian.
bsToFloating :: (Floating f, Storable f, Storable a)
                => (a -> a) -> BS.ByteString -> f
bsToFloating byteSwap bs = unsafeDupablePerformIO $ unsafeUseAsCString bs castBs
  where
#if __BYTE_ORDER == __LITTLE_ENDIAN
    castBs chrPtr = do
      w <- peek (castPtr chrPtr)
      poke (castPtr chrPtr) (byteSwap w)
      peek (castPtr chrPtr)
#else
    castBs = peek . castPtr
#endif
