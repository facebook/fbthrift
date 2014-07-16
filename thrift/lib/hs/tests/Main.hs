{-# LANGUAGE RecordWildCards #-}
{-# LANGUAGE NegativeLiterals #-}
{-# LANGUAGE OverloadedStrings #-}

module Main where

import Control.Monad
import Data.Functor
import Data.IORef
import Prelude
import Test.QuickCheck
import Test.QuickCheck.Property
import System.Exit

import Thrift.Transport
import Thrift.Transport.Framed
import Thrift.Protocol.Binary

import Hs_test_Types
import Util

propRoundTrip :: TestStruct -> Property
propRoundTrip cf = morallyDubiousIOProperty $ do
  ref <- newIORef ""
  t <- openFramedTransport (TestTransport ref)
  let p = BinaryProtocol t
  write_TestStruct p cf
  tFlush t
  (==cf) <$> read_TestStruct p

main :: IO ()
main = quickCheckResult propRoundTrip >>= \result -> case result of
  Success{..} -> exitSuccess
  _ -> exitFailure
