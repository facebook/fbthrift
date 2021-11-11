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

{-# LANGUAGE CPP #-}
{-# LANGUAGE FlexibleInstances #-}
{-# LANGUAGE MultiParamTypeClasses #-}
{-# LANGUAGE ScopedTypeVariables #-}
{-# LANGUAGE TypeSynonymInstances #-}
{-# OPTIONS_GHC -fno-warn-orphans #-}
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

module Thrift.Transport.Handle
    ( module Thrift.Transport
    , Port
    , HandleSource(..)
    ) where

import Control.Exception ( catch, throw )
import Data.ByteString.Internal (c2w)
#if __GLASGOW_HASKELL__ < 710
import Data.Functor
#endif

#if MIN_VERSION_network(2,7,0)
import Data.Maybe
import Network.Socket
#else
import Network
#endif

import System.IO
import System.IO.Error ( isEOFError )

import Thrift.Transport

import qualified Data.ByteString.Lazy as LBS
#if __GLASGOW_HASKELL__ < 710
import Data.Monoid
#endif

instance Transport Handle where
    tIsOpen = hIsOpen
    tClose = hClose
    tRead h n = LBS.hGet h n `catch` handleEOF mempty
    tPeek h = (Just . c2w <$> hLookAhead h) `catch` handleEOF Nothing
    tWrite = LBS.hPut
    tFlush = hFlush


-- | Type class for all types that can open a Handle. This class is used to
-- replace tOpen in the Transport type class.
class HandleSource s where
    hOpen :: s -> IO Handle

instance HandleSource FilePath where
    hOpen s = openFile s ReadWriteMode

type Port = String

instance HandleSource (HostName, Port) where
#if MIN_VERSION_network(2,7,0)
    hOpen (h,p) = do
        let hints = defaultHints{addrFamily = AF_INET}
        addr <- fromMaybe (error "getAddrInfo") . listToMaybe <$>
            getAddrInfo (Just hints) (Just h) (Just p)
        s <- socket (addrFamily addr) (addrSocketType addr) (addrProtocol addr)
        connect s $ addrAddress addr
        socketToHandle s ReadWriteMode
#else
    hOpen (h,p) = connectTo h (PortNumber $ read p)
#endif

handleEOF :: a -> IOError -> IO a
handleEOF a e = if isEOFError e
    then return a
    else throw $ TransportExn "TChannelTransport: Could not read" TE_UNKNOWN
