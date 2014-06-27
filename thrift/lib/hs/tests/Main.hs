{-# LANGUAGE RecordWildCards #-}
{-# LANGUAGE NegativeLiterals #-}
{-# LANGUAGE OverloadedStrings #-}

module Main where

import Control.Monad
import Control.Monad.IO.Class
import Data.Functor
import Data.IORef
import Prelude
import Test.QuickCheck
import Test.QuickCheck.Property
import System.Exit
import qualified Data.ByteString.Lazy as LBS
import qualified Data.Text.Lazy as LT

import Thrift.Transport
import Thrift.Protocol.Binary

data TestTransport = TestTransport (IORef LBS.ByteString)

instance Transport TestTransport where
  tIsOpen _ = return True
  tClose _ = return ()
  tPeek (TestTransport t) = liftIO $ LBS.head <$> readIORef t
  tRead (TestTransport t) i = liftIO $ do
    s <- readIORef t
    let (hd, tl) = LBS.splitAt (fromIntegral i) s
    writeIORef t tl
    return hd
  tWrite (TestTransport t) bs = liftIO $
    readIORef t >>= writeIORef t . (`LBS.append` bs)
  tFlush _ = return ()

prop_roundtrip
  :: (Eq a, Show a)
     => (BinaryProtocol TestTransport -> a -> IO ())
     -> (BinaryProtocol TestTransport -> IO a)
     -> a
     -> Property
prop_roundtrip write read a = morallyDubiousIOProperty $ do
  ref <- newIORef ""
  let t = BinaryProtocol (TestTransport ref)
  write t a
  b <- read t
  return (a == b)


main :: IO ()
main = do
  results <- sequence
    [ qcrt writeBool readBool
    , qcrt writeByte readByte
    , qcrt writeI16 readI16
    , qcrt writeI32 readI32
    , qcrt writeI64 readI64
    , qcrt writeFloat readFloat
    , qcrt writeDouble readDouble
    , quickCheckWithResult args $
      prop_roundtrip writeString readString . LT.pack
    , quickCheckWithResult args $
      prop_roundtrip writeBinary readBinary . LBS.pack
    ]
  if all success results
    then exitSuccess
    else exitFailure
  where
    qcrt w r = quickCheckWithResult args $ prop_roundtrip w r
    args = Args Nothing 50 10 100 True
    success Success{..} = True
    success _ = False
