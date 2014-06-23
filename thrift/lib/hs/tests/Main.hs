{-# LANGUAGE NegativeLiterals #-}
{-# LANGUAGE OverloadedStrings #-}

module Main where

import Control.Monad
import Control.Monad.IO.Class
import Data.Binary
import Data.IORef
import Prelude
import Test.HUnit

import qualified Data.ByteString.Lazy as LBS

import Thrift.Transport
import Thrift.Protocol.Binary

data TestTransport = TestTransport (IORef LBS.ByteString)

instance Transport TestTransport where
  tIsOpen _ = return True
  tClose _ = return ()
  tPeek (TestTransport t) = liftIO $ LBS.head `liftM` readIORef t
  tRead (TestTransport t) i = liftIO $ do
    s <- readIORef t
    let (hd, tl) = LBS.splitAt (fromIntegral i) s
    writeIORef t tl
    return hd
  tWrite (TestTransport t) bs = liftIO $
    readIORef t >>= writeIORef t . (`LBS.append` bs)
  tFlush _ = return ()

roundtripTest
  :: (Binary a, Eq a, Show a)
     => (BinaryProtocol TestTransport -> a -> IO ())
     -> (BinaryProtocol TestTransport -> IO a)
     -> a
     -> Test
roundtripTest write read a = TestCase $ do
  ref <- newIORef ""
  let t = BinaryProtocol (TestTransport ref)
  write t a
  b <- read t
  assertEqual "same result" a b

main :: IO Counts
main = runTestTT $ TestList [
  TestList $ map (roundtripTest writeBool readBool) [True, False],
  TestList $ map (roundtripTest writeByte readByte) [0, 10, 127, -128],
  TestList $ map (roundtripTest writeI16 readI16) [0, 10, 32767, -32767],
  TestList $ map (roundtripTest writeI32 readI32) [0, 10, 111111, -574839],
  TestList $ map (roundtripTest writeI64 readI64) [0, 10, 15684615, -839283],
  TestList $ map (roundtripTest writeFloat readFloat) [0, 1.001e76, 3.14159],
  TestList $ map (roundtripTest writeFloat readFloat) [0, 1.001e76, 3.14159],
  roundtripTest writeBinary readBinary "Hello World"
  ]
