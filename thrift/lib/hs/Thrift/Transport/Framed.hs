{-# LANGUAGE FlexibleInstances #-}
{-# LANGUAGE MultiParamTypeClasses #-}
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

module Thrift.Transport.Framed
    ( module Thrift.Transport
    , FramedTransport
    , openFramedTransport
    ) where

import Thrift.Transport
import Thrift.Transport.HttpClient hiding (HttpClient(..))

import Control.Monad (liftM)
import Data.Int (Int32)
import Control.Monad.IO.Class
import qualified Data.Binary as B
import qualified Data.ByteString.Lazy as LBS


-- | FramedTransport wraps a given transport in framed mode.
data FramedTransport t = FramedTransport {
      wrappedTrans :: t,           -- ^ Underlying transport.
      writeBuffer  :: WriteBuffer, -- ^ Write buffer.
      readBuffer   :: ReadBuffer   -- ^ Read buffer.
    }

-- | Create a new framed transport which wraps the given transport.
openFramedTransport :: (MonadIO m, Transport t) => t -> m (FramedTransport t)
openFramedTransport trans = liftIO $ do
  wbuf <- newWriteBuffer
  rbuf <- newReadBuffer
  return FramedTransport{ wrappedTrans = trans, writeBuffer = wbuf, readBuffer = rbuf }

instance Transport t => Transport (FramedTransport t) where

    tClose = tClose . wrappedTrans

    tRead trans n = do
      -- First, check the read buffer for any data.
      bs <- liftIO $ readBuf (readBuffer trans) n
      if LBS.null bs
         then
         -- When the buffer is empty, read another frame from the
         -- underlying transport.
           do len <- readFrame trans
              if len > 0
                 then tRead trans 1
                 else return bs
         else return bs
    tPeek trans = do
      bs <- liftIO $ readBuf (readBuffer trans) 1
      if LBS.null bs
        then do
          len <- readFrame trans
          if len > 0
             then LBS.head `liftM` tRead trans 1
             else return $ LBS.head bs
          else return $ LBS.head bs
    tWrite trans s = liftIO $ writeBuf (writeBuffer trans) s

    tFlush trans = do
      bs <- liftIO $ flushBuf (writeBuffer trans)
      let szBs = B.encode $ (fromIntegral $ LBS.length bs :: Int32)
      tWrite (wrappedTrans trans) szBs
      tWrite (wrappedTrans trans) bs
      tFlush (wrappedTrans trans)

    tIsOpen = tIsOpen . wrappedTrans

readFrame :: (MonadIO m, Transport t) => FramedTransport t -> m Int
readFrame trans = do
  -- Read and decode the frame size.
  szBs <- tRead (wrappedTrans trans) 4
  let sz = fromIntegral (B.decode szBs :: Int32)

  -- Read the frame and stuff it into the read buffer.
  bs   <- tRead (wrappedTrans trans) sz
  liftIO $ fillBuf (readBuffer trans) bs

  -- Return the frame size so that the caller knows whether to expect
  -- something in the read buffer or not.
  return sz
