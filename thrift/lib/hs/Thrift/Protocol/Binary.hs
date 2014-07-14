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
{-# LANGUAGE ScopedTypeVariables #-}

module Thrift.Protocol.Binary
    ( module Thrift.Protocol
    , BinaryProtocol(..)
    ) where

import Control.Exception ( throw )
import Control.Monad
import Control.Monad.IO.Class

import Data.Bits
import Data.ByteString.Builder
import Data.Functor
import Data.Int
import Data.Word
import Data.Text.Lazy.Encoding ( decodeUtf8, encodeUtf8 )

import Foreign.Ptr
import Foreign.Storable

import Thrift.Protocol
import Thrift.Transport
import Thrift.Types

import qualified Data.Binary as Binary
import qualified Data.ByteString.Lazy as LBS
import qualified Data.Text.Lazy as LT
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

    writeMessage p (n, t, s) = do
        writeBinaryValue p (TI32 (version_1 .|. fromIntegral (fromEnum t)))
        writeBinaryValue p (TString $ encodeUtf8 n)
        writeBinaryValue p (TI32 s)

    readMessage p = do
      TI32 ver <- readBinaryValue p T_I32
      if ver .&. version_mask /= version_1
        then throw $ ProtocolExn PE_BAD_VERSION "Missing version identifier"
        else do
          TString s <- readBinaryValue p T_STRING
          TI32 sz <- readBinaryValue p T_I32
          return (decodeUtf8 s, toEnum $ fromIntegral $ ver .&. 0xFF, sz)

    writeVal = writeBinaryValue
    readVal p = readBinaryValue p T_STRUCT

-- | Writing Functions
writeBinaryValue :: Transport t => BinaryProtocol t -> ThriftVal -> IO ()
writeBinaryValue p (TStruct fields) = writeBinaryStruct p fields
writeBinaryValue p (TMap ky vt entries) = do
  writeType p ky
  writeType p vt
  let len :: Int32 = fromIntegral (length entries)
  tWrite (getTransport p) (Binary.encode len)
  writeBinaryMap p entries
writeBinaryValue p (TList ty entries) = do
  writeType p ty
  let len :: Int32 = fromIntegral (length entries)
  tWrite (getTransport p) (Binary.encode len)
  writeBinaryList p entries
writeBinaryValue p (TSet ty entries) = do
  writeType p ty
  let len :: Int32 = fromIntegral (length entries)
  tWrite (getTransport p) (Binary.encode len)
  writeBinaryList p entries
writeBinaryValue p (TBool b) =
  tWrite (getTransport p) $ LBS.singleton $ toEnum $ if b then 1 else 0
writeBinaryValue p (TByte b) = tWrite (getTransport p) $ Binary.encode b
writeBinaryValue p (TI16 i) = tWrite (getTransport p) $ Binary.encode i
writeBinaryValue p (TI32 i) = tWrite (getTransport p) $ Binary.encode i
writeBinaryValue p (TI64 i) = tWrite (getTransport p) $ Binary.encode i
writeBinaryValue p (TFloat f) =
  tWrite (getTransport p) $ toLazyByteString $ floatBE f
writeBinaryValue p (TDouble d) =
  tWrite (getTransport p) $ toLazyByteString $ doubleBE d
writeBinaryValue p (TString s) = tWrite t (Binary.encode len) >> tWrite t s
  where
    t = getTransport p
    len :: Int32 = fromIntegral (LBS.length s)

writeBinaryStruct :: Transport t
                     => BinaryProtocol t
                     -> [(Int16, LT.Text, ThriftVal)]
                     -> IO ()
writeBinaryStruct p ((fid, _key, val):fields) = do
  let t = getTransport p
  writeTypeOf p val
  tWrite t (Binary.encode fid)
  writeBinaryValue p val
  tFlush t
  writeBinaryStruct p fields
writeBinaryStruct p [] = writeType p T_STOP

writeBinaryMap :: Transport t
                  => BinaryProtocol t
                  -> [(ThriftVal, ThriftVal)]
                  -> IO ()
writeBinaryMap p ((key, val):entries) = do
  writeBinaryValue p key
  writeBinaryValue p val
  writeBinaryMap p entries
writeBinaryMap _ [] = return ()

writeBinaryList :: Transport t => BinaryProtocol t -> [ThriftVal] -> IO ()
writeBinaryList p (val:entries) = do
  writeBinaryValue p val
  writeBinaryList p entries
writeBinaryList _ [] = return ()

-- | Reading Functions
readBinaryValue :: Transport t => BinaryProtocol t -> ThriftType -> IO ThriftVal
readBinaryValue p T_STRUCT = TStruct <$> readBinaryStruct p
readBinaryValue p T_MAP = do
  kt <- readType p
  vt <- readType p
  n <- Binary.decode <$> tReadAll (getTransport p) 4
  TMap kt vt <$> readBinaryMap p kt vt n
readBinaryValue p T_LIST = do
  t <- readType p
  n <- Binary.decode <$> tReadAll (getTransport p) 4
  TList t <$> readBinaryList p t n
readBinaryValue p T_SET = do
  t <- readType p
  n <- Binary.decode <$> tReadAll (getTransport p) 4
  TSet t <$> readBinaryList p t n
readBinaryValue p T_BOOL = TBool . (/=0) . bsToI8 <$> tRead (getTransport p) 1
  where
    bsToI8 :: LBS.ByteString -> Int8
    bsToI8 = Binary.decode
readBinaryValue p T_BYTE =
  TByte . Binary.decode <$> tReadAll (getTransport p) 1
readBinaryValue p T_I16 =
  TI16 . Binary.decode <$> tReadAll (getTransport p) 2
readBinaryValue p T_I32 =
  TI32 . Binary.decode <$> tReadAll (getTransport p) 4
readBinaryValue p T_I64 =
  TI64 . Binary.decode <$> tReadAll (getTransport p) 8
readBinaryValue p T_FLOAT =
  TFloat <$> (tRead (getTransport p) 4 >>= bsToFloating byteSwap32)
readBinaryValue p T_DOUBLE =
  TDouble <$> (tRead (getTransport p) 8 >>= bsToFloating byteSwap64)
readBinaryValue p T_STRING = do
  let t = getTransport p
  i :: Int32  <- Binary.decode <$> tReadAll t 4
  bs <- tReadAll t (fromIntegral i)
  return $ TString bs
readBinaryValue _ ty = error $ "Cannot read value of type " ++ show ty

readBinaryStruct :: Transport t
                    => BinaryProtocol t
                    -> IO [(Int16, LT.Text, ThriftVal)]
readBinaryStruct p = do
  t <- readType p
  case t of
    T_STOP -> return []
    _ -> do
      n <- Binary.decode <$> tReadAll (getTransport p) 2
      v <- readBinaryValue p t
      ((n, "", v) :) <$> readBinaryStruct p

readBinaryMap :: Transport t
                 => BinaryProtocol t
                 -> ThriftType
                 -> ThriftType
                 -> Int32
                 -> IO [(ThriftVal, ThriftVal)]
readBinaryMap p kt vt n | n <= 0 = return []
                        | otherwise = do
  k <- readBinaryValue p kt
  v <- readBinaryValue p vt
  ((k,v) :) <$> readBinaryMap p kt vt (n-1)

readBinaryList :: Transport t
                  => BinaryProtocol t
                  -> ThriftType
                  -> Int32
                  -> IO [ThriftVal]
readBinaryList p ty n | n <= 0 = return []
                      | otherwise = liftM2 (:) (readBinaryValue p ty)
                                    (readBinaryList p ty (n-1))



-- | Write a type as a byte
writeType :: (Protocol p, MonadIO m, Transport t) => p t -> ThriftType -> m ()
writeType p t = tWrite (getTransport p) (Binary.encode w)
  where
    w :: Word8
    w = fromIntegral $ fromEnum t

-- | Write type of a ThriftVal as a byte
writeTypeOf :: (Protocol p, MonadIO m, Transport t) => p t -> ThriftVal -> m ()
writeTypeOf p v = writeType p $ case v of
  TStruct{} -> T_STRUCT
  TMap{} -> T_MAP
  TList{} -> T_LIST
  TSet{} -> T_SET
  TBool{} -> T_BOOL
  TByte{} -> T_BYTE
  TI16{} -> T_I16
  TI32{} -> T_I32
  TI64{} -> T_I64
  TString{} -> T_STRING
  TFloat{} -> T_FLOAT
  TDouble{} -> T_DOUBLE

-- | Read a byte as though it were a ThriftType
readType :: (Protocol p, MonadIO m, Transport t) => p t -> m ThriftType
readType p = do
    b <- tReadAll (getTransport p) 1
    let w :: Word8 = Binary.decode b
    return $ toEnum $ fromIntegral w

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
