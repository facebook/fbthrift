{-# LANGUAGE BangPatterns #-}
{-# LANGUAGE OverloadedStrings #-}
{-# LANGUAGE RecordWildCards #-}

module IntegrationTests where

import Data.Functor
import Data.IORef
import Foreign.C.String
import Foreign.C.Types
import Foreign.Marshal.Alloc
import Foreign.Ptr
import Foreign.Storable
import System.Exit
import Test.QuickCheck
import Test.QuickCheck.Property
import qualified Data.ByteString as BS
import qualified Data.ByteString.Lazy as LBS

import Thrift.Protocol.Binary
import Thrift.Protocol.JSON
import Interface
import Util

import Hs_test_Types


-- | Serialize a TestStruct from C++ and deserialize in Haskell
propCToHs :: Protocol p
             => (TestTransport -> p TestTransport)
             -> (Ptr SerializedResult -> Ptr TestStruct -> IO ())
             -> TestStruct
             -> Property
propCToHs pCons cToHS struct@TestStruct{..} = morallyDubiousIOProperty $ do
  structPtr <- c_newStructPtr
  poke structPtr struct
  alloca $ \sr -> do
    cToHS sr structPtr
    SR chrPtr len <- peek sr
    bs <- LBS.fromStrict <$> BS.packCStringLen (chrPtr, len)
    ref <- newIORef bs
    let p = pCons (TestTransport ref)
    !result <- read_TestStruct p
    c_deleteSResult sr
    c_freeTestStruct structPtr
    return $ result == struct

-- | Serialize a TestStruct in Haskell and deserialize in C++
propHsToC :: Protocol p
             => (TestTransport -> p TestTransport)
             -> (CString -> CInt -> IO (Ptr TestStruct))
             -> TestStruct
             -> Property
propHsToC pCons hsToC struct@TestStruct{..} = morallyDubiousIOProperty $ do
  ref <- newIORef ""
  let p = pCons (TestTransport ref)
  write_TestStruct p struct
  bs <- LBS.toStrict <$> readIORef ref
  BS.useAsCStringLen bs $ \(cStr, len) -> do
    structPtr <- hsToC cStr (CInt $ fromIntegral len)
    !result <- peek structPtr
    c_freeTestStruct structPtr
    return $ struct == result

main :: IO ()
main = do
  results <- sequence
    [ quickCheckWithResult args (propCToHs BinaryProtocol c_serializeBinary)
    , quickCheckWithResult args (propHsToC BinaryProtocol c_deserializeBinary)
    , quickCheckWithResult args (propCToHs JSONProtocol c_serializeJSON)
    , quickCheckWithResult args (propHsToC JSONProtocol c_deserializeJSON)
    ]
  if all successful results
  then exitSuccess
  else exitFailure
  where
    successful Success{} = True
    successful _ = False
    args = Args Nothing 100 10 100 True
