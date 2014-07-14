{-# LANGUAGE RecordWildCards #-}
{-# LANGUAGE NegativeLiterals #-}
{-# LANGUAGE OverloadedStrings #-}

module Main where

import Control.Monad
import Control.Monad.IO.Class
import Data.Functor
import Data.IORef
import Data.Monoid
import Prelude
import Test.QuickCheck
import Test.QuickCheck.Property
import System.Exit
import qualified Data.ByteString.Lazy as LBS

import Thrift.Transport
import Thrift.Transport.Framed
import Thrift.Protocol.Binary

import Hs_test_Types

data TestTransport = TestTransport (IORef LBS.ByteString)

instance Transport TestTransport where
  tIsOpen _ = return True
  tClose _ = return ()
  tPeek (TestTransport t) = liftIO $ (fmap fst . LBS.uncons) <$> readIORef t
  tRead (TestTransport t) i = liftIO $ do
    (hd, tl) <- LBS.splitAt (fromIntegral i) <$> readIORef t
    writeIORef t tl
    return hd
  tWrite (TestTransport t) bs = liftIO $ modifyIORef' t (<> bs)
  tFlush _ = return ()


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
