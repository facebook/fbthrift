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
{-# LANGUAGE ScopedTypeVariables #-}
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

module Thrift.Server
    ( runThreadedServer
    ) where

import Control.Concurrent ( forkIO )
import Control.Exception
import Control.Monad ( forever, when )

import Network.Socket (Socket)

import Thrift
import Thrift.Transport.Handle()


-- | A threaded sever that is capable of using any Transport or Protocol
-- instances.
runThreadedServer :: (Transport t, Protocol i, Protocol o)
                  => (Socket -> IO (i t, o t))
                  -> h
                  -> (h -> (i t, o t) -> IO Bool)
                  -> Socket
                  -> IO a
runThreadedServer accepter hand proc_ socket =
  acceptLoop (accepter socket) (proc_ hand)

acceptLoop :: IO t -> (t -> IO Bool) -> IO a
acceptLoop accepter proc_ = forever $
    do ps <- accepter
       forkIO $ handle (\(_ :: SomeException) -> return ())
                  (loop $ proc_ ps)
  where loop m = do { continue <- m; when continue (loop m) }
