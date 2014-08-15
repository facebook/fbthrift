{-# LANGUAGE OverloadedStrings #-}

module IntegrationTests where

import Control.Exception
import Data.Functor
import Foreign.Ptr
import Foreign.Storable
import Test.QuickCheck
import Test.QuickCheck.Property

import Thrift.Protocol.Binary
import Thrift.Protocol.Compact
import Thrift.Protocol.SimpleJSON
import Thrift.Transport
import Interface
import Util

import Hs_test_Types


-- | Serialize a TestStruct from C++ and deserialize in Haskell
propCToHs :: Protocol p
             => (Ptr MockTransport -> p (Ptr MockTransport))
             -> (Ptr MockTransport -> Ptr TestStruct -> IO ())
             -> TestStruct
             -> Property
propCToHs pCons cToHS struct = morallyDubiousIOProperty $
  bracket c_newStructPtr c_freeTestStruct $ \structPtr ->
  bracket c_openMT tClose $ \mt -> do
    poke structPtr struct
    cToHS mt structPtr
    (== struct) <$> read_TestStruct (pCons mt)

-- | Serialize a TestStruct in Haskell and deserialize in C++
propHsToC :: Protocol p
             => (Ptr MockTransport -> p (Ptr MockTransport))
             -> (Ptr MockTransport -> IO (Ptr TestStruct))
             -> TestStruct
             -> Property
propHsToC pCons hsToC struct = morallyDubiousIOProperty $
  bracket c_openMT tClose $ \mt -> do
    write_TestStruct (pCons mt) struct
    bracket (hsToC mt) c_freeTestStruct $ \structPtr ->
      (== struct) <$> peek structPtr

main :: IO ()
main = aggregateResults
  [ quickCheckWithResult args (propCToHs BinaryProtocol c_serializeBinary)
  , quickCheckWithResult args (propHsToC BinaryProtocol c_deserializeBinary)
  , quickCheckWithResult args $
    propCToHs SimpleJSONProtocol c_serializeSimpleJSON
  , quickCheckWithResult args $
    propHsToC SimpleJSONProtocol c_deserializeSimpleJSON
  , quickCheckWithResult args (propCToHs CompactProtocol c_serializeCompact)
  , quickCheckWithResult args (propHsToC CompactProtocol c_deserializeCompact)
  ]
  where args = Args Nothing 100 10 100 True
