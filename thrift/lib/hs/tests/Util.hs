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

{-# OPTIONS_GHC -fno-warn-orphans #-}
{-# LANGUAGE CPP #-}
{-# LANGUAGE OverloadedStrings #-}
module Util
       ( TestTransport(..)
       , propRoundTrip
       , propRoundTripMessage
       , aggregateResults
       ) where

import Control.Monad
import Data.Functor
import Data.Int
import Data.IORef
#if __GLASGOW_HASKELL__ < 804
import Data.Monoid
#endif
import Prelude
import System.Exit
import Test.QuickCheck as QC
import qualified Data.ByteString.Lazy as LBS
import qualified Data.Text.Lazy as LT

import Thrift.Arbitraries ()
import Thrift.Protocol
import Thrift.Transport
import Thrift.Transport.Framed
import Thrift.Types

import Hs_test_Types

data TestTransport = TestTransport (IORef LBS.ByteString)

instance Transport TestTransport where
  tIsOpen _ = return True
  tClose _ = return ()
  tPeek (TestTransport t) = (fmap fst . LBS.uncons) <$> readIORef t
  tRead (TestTransport t) i = do
    s <- readIORef t
    let (hd, tl) = LBS.splitAt (fromIntegral i) s
    writeIORef t tl
    return hd
  tWrite (TestTransport t) bs = modifyIORef' t (<> bs)
  tFlush _ = return ()

propRoundTrip :: Protocol p
              => (FramedTransport TestTransport
                  -> p (FramedTransport TestTransport))
              -> TestStruct
              -> Property
propRoundTrip pcons cf = ioProperty $ do
  ref <- newIORef ""
  t <- openFramedTransport (TestTransport ref)
  let p = pcons t
  write_TestStruct p cf
  tFlush t
  (==cf) <$> read_TestStruct p

propRoundTripMessage :: Protocol p
                     => (TestTransport -> p TestTransport)
                     -> (LT.Text, MessageType, Int32)
                     -> Property
propRoundTripMessage pcons args = ioProperty $ do
  ref <- newIORef ""
  let p = pcons (TestTransport ref)
  writeMessage p args (return ())
  (==args) <$> readMessage p return

aggregateResults :: [IO QC.Result] -> IO ()
aggregateResults qcs = do
  results <- sequence qcs
  if all successful results
    then exitSuccess
    else exitFailure
  where
    successful Success{} = True
    successful _ = False
