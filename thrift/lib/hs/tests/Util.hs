{-# OPTIONS_GHC -fno-warn-orphans #-}
module Util
       ( TestTransport(..)
       ) where

import Control.Monad
import Data.Functor
import Data.IORef
import Data.Monoid
import Data.Vector
import Prelude
import Test.QuickCheck
import qualified Data.ByteString.Lazy as LBS

import Thrift.Transport

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

instance Arbitrary LBS.ByteString where
  arbitrary = LBS.pack <$> arbitrary

instance Arbitrary a => Arbitrary (Vector a) where
  arbitrary = fromList <$> arbitrary
