{-# LANGUAGE OverloadedStrings #-}

module JSONTests where

import Data.Functor
import Data.Int
import Data.IORef
import Data.Text.Lazy
import Prelude
import Test.QuickCheck
import Test.QuickCheck.Property
import System.Exit

import Thrift.Transport
import Thrift.Transport.Framed
import Thrift.Types
import Thrift.Protocol.JSON

import Hs_test_Types
import Util

propRoundTrip :: TestStruct -> Property
propRoundTrip cf = morallyDubiousIOProperty $ do
  ref <- newIORef ""
  t <- openFramedTransport (TestTransport ref)
  let p = JSONProtocol t
  write_TestStruct p cf
  tFlush t
  (==cf) <$> read_TestStruct p

propRoundTripMessage :: (Text, MessageType, Int32) -> Property
propRoundTripMessage args = morallyDubiousIOProperty $ do
  ref <- newIORef ""
  let p = JSONProtocol (TestTransport ref)
  writeMessage p args
  (==args) <$> readMessage p

main :: IO ()
main = do
  r1 <- quickCheckResult propRoundTrip
  r2 <- quickCheckResult propRoundTripMessage
  case (r1, r2) of
    (Success{}, Success{}) -> exitSuccess
    _ -> exitFailure
