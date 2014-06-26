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

{-# LANGUAGE CPP #-}
{-# LANGUAGE ExistentialQuantification #-}
{-# LANGUAGE OverloadedStrings #-}

module Thrift.Protocol.Binary
    ( module Thrift.Protocol
    , BinaryProtocol(..)
    ) where

import Control.Exception ( throw )
import Control.Monad ( liftM )
import Control.Monad.IO.Class

import qualified Data.Binary
import Data.Bits
import Data.ByteString.Builder
import Data.Int
import Data.Word
import Data.Text.Lazy.Encoding ( decodeUtf8, encodeUtf8 )

import Foreign.Ptr
import Foreign.Storable

import Thrift.Protocol
import Thrift.Transport

import qualified Data.ByteString.Lazy as LBS
import Data.ByteString.Unsafe

version_mask :: Int32
version_mask = fromIntegral (0xffff0000 :: Word32)

version_1 :: Int32
version_1    = fromIntegral (0x80010000 :: Word32)

data BinaryProtocol a = BinaryProtocol a


-- NOTE: Reading and Writing functions rely on Data.Binary to encode and decode
-- data.  Data.Binary assumes that the binary values it is encoding to and
-- decoding from are in BIG ENDIAN format, and converts the endianness as
-- necessary to match the local machine.
instance Protocol BinaryProtocol where
    getTransport (BinaryProtocol t) = t

    writeMessageBegin p (n, t, s) = do
        writeI32 p (version_1 .|. (fromIntegral $ fromEnum t))
        writeString p n
        writeI32 p s
    writeMessageEnd _ = return ()

    writeStructBegin _ _ = return ()
    writeStructEnd _ = return ()
    writeFieldBegin p (_, t, i) = writeType p t >> writeI16 p i
    writeFieldEnd _ = return ()
    writeFieldStop p = writeType p T_STOP
    writeMapBegin p (k, v, n) = writeType p k >> writeType p v >> writeI32 p n
    writeMapEnd _ = return ()
    writeListBegin p (t, n) = writeType p t >> writeI32 p n
    writeListEnd _ = return ()
    writeSetBegin p (t, n) = writeType p t >> writeI32 p n
    writeSetEnd _ = return ()

    writeBool p b = tWrite (getTransport p) $ LBS.singleton $ toEnum $ if b then 1 else 0
    writeByte p b = tWrite (getTransport p) $ Data.Binary.encode b
    writeI16 p b = tWrite (getTransport p) $ Data.Binary.encode b
    writeI32 p b = tWrite (getTransport p) $ Data.Binary.encode b
    writeI64 p b = tWrite (getTransport p) $ Data.Binary.encode b
    writeFloat p  = tWrite (getTransport p) . toLazyByteString . floatBE
    writeDouble p = tWrite (getTransport p) . toLazyByteString . doubleBE
    writeString p s = writeI32 p (fromIntegral $ LBS.length s') >> tWrite (getTransport p) s'
      where
        s' = encodeUtf8 s
    writeBinary p s = writeI32 p (fromIntegral $ LBS.length s) >> tWrite (getTransport p) s

    readMessageBegin p = do
        ver <- readI32 p
        if (ver .&. version_mask /= version_1)
            then throw $ ProtocolExn PE_BAD_VERSION "Missing version identifier"
            else do
              s <- readString p
              sz <- readI32 p
              return (s, toEnum $ fromIntegral $ ver .&. 0xFF, sz)
    readMessageEnd _ = return ()
    readStructBegin _ = return ""
    readStructEnd _ = return ()
    readFieldBegin p = do
        t <- readType p
        n <- if t /= T_STOP then readI16 p else return 0
        return ("", t, n)
    readFieldEnd _ = return ()
    readMapBegin p = do
        kt <- readType p
        vt <- readType p
        n <- readI32 p
        return (kt, vt, n)
    readMapEnd _ = return ()
    readListBegin p = do
        t <- readType p
        n <- readI32 p
        return (t, n)
    readListEnd _ = return ()
    readSetBegin p = do
        t <- readType p
        n <- readI32 p
        return (t, n)
    readSetEnd _ = return ()

    readBool p = (== 1) `liftM` readByte p

    readByte p = do
        bs <- tReadAll (getTransport p) 1
        return $ Data.Binary.decode bs

    readI16 p = do
        bs <- tReadAll (getTransport p) 2
        return $ Data.Binary.decode bs

    readI32 p = do
        bs <- tReadAll (getTransport p) 4
        return $ Data.Binary.decode bs

    readI64 p = do
        bs <- tReadAll (getTransport p) 8
        return $ Data.Binary.decode bs

    readFloat p = tRead (getTransport p) 4 >>= bsToFloating byteSwap32

    readDouble p = tRead (getTransport p) 8 >>= bsToFloating byteSwap64

    readString p = do
        i <- readI32 p
        decodeUtf8 `liftM` tReadAll (getTransport p) (fromIntegral i)

    readBinary p = do
        i <- readI32 p
        tReadAll (getTransport p) (fromIntegral i)


-- | Write a type as a byte
writeType :: (Protocol p, MonadIO m, Transport t) => p t -> ThriftType -> m ()
writeType p t = writeByte p (fromIntegral $ fromEnum t)

-- | Read a byte as though it were a ThriftType
readType :: (Protocol p, MonadIO m, Transport t) => p t -> m ThriftType
readType p = do
    b <- readByte p
    return $ toEnum $ fromIntegral b

-- | Converts a ByteString to a Floating point number
-- The ByteString is assumed to be encoded in network order (Big Endian)
-- therefore the behavior of this function varies based on whether the local
-- machine is big endian or little endian.
bsToFloating :: (Floating f, MonadIO m, Storable f, Storable a)
                => (a -> a) -> LBS.ByteString -> m f
bsToFloating byteSwap bs = liftIO $ unsafeUseAsCString (LBS.toStrict bs) castBs
  where
#if __BYTE_ORDER == __LITTLE_ENDIAN
    castBs chrPtr = do
      w <- peek (castPtr chrPtr)
      poke (castPtr chrPtr) (byteSwap w)
      peek (castPtr chrPtr)
#else
    castBs = peek . castPtr
#endif
