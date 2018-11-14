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

{-# LANGUAGE OverloadedStrings, ScopedTypeVariables #-}

module ClientServerTest where

import Control.Concurrent
import Control.Exception
import Network as N
import Network.Socket as NS
import System.IO
import Test.HUnit
import qualified Data.HashMap.Strict as Map
import qualified Data.HashSet as Set
import qualified Data.Vector as Vector

import ThriftTest_Types
import qualified ThriftTest_Client as Client

import Thrift.Transport
import Thrift.Transport.Handle
import Thrift.Protocol
import Thrift.Protocol.Binary
import Thrift.Protocol.Compact
import Thrift.Protocol.SimpleJSON

import TestServer

runClient :: (Protocol p, Transport t) => p t -> Test
runClient p =
  let prot = (p,p)
      struct = Xtruct{ xtruct_string_thing = "Zero"
                     , xtruct_byte_thing   = 1
                     , xtruct_i32_thing    = -3
                     , xtruct_i64_thing    = -5
                     }
      nested = Xtruct2{ xtruct2_byte_thing   = 1
                      , xtruct2_struct_thing = struct
                      , xtruct2_i32_thing    = 5
                      }
      theMap = Map.fromList $ map (\i -> (i, i-10)) [1..5]
      theSet = Set.fromList [-2..3]
      theList = Vector.fromList [-2..3]
  in TestList $ map TestCase
     -- VOID Test
     [ Client.testVoid prot >>= assertEqual "Void Test" ()

     -- String Test
     , Client.testString prot "Test" >>= assertEqual "String Test" "Test"

     -- Byte Test
     , Client.testByte prot 1 >>= assertEqual "Byte Test" 1

     -- I32 Test
     , Client.testI32 prot (-1) >>= assertEqual "i32 Test" (-1)

     -- I64 Test
     , Client.testI64 prot (-34359738368) >>=
       assertEqual "i64 Test" (-34359738368)

     -- Double Test
     , do dub <- Client.testDouble prot (-5.2098523)
          assertBool "Double Test" (abs (dub + 5.2098523) < 0.001)

     -- Struct Test
     , Client.testStruct prot struct >>= assertEqual "Struct Test" struct

     -- Nested Struct Test
     , Client.testNest prot nested >>= assertEqual "Nested Struct Test" nested

     -- Map Test
     , Client.testMap prot theMap >>= assertEqual "Map Test" theMap

     -- Set Test
     , Client.testSet prot theSet >>= assertEqual "Set Test" theSet

     -- List Test
     , Client.testList prot theList >>= assertEqual "List Test" theList

     -- Enum Tests
     , Client.testEnum prot ONE >>= assertEqual "Enum Test 1" ONE
     , Client.testEnum prot TWO >>= assertEqual "Enum Test 1" TWO
     , Client.testEnum prot FIVE >>= assertEqual "Enum Test 1" FIVE

     -- Typedef Test
     , Client.testTypedef prot 309858235082523 >>=
       assertEqual "Typedef Test" 309858235082523

     -- Exception Tests
     , do exn <- try $ Client.testException prot "Xception"
          case exn of
            Left (Xception _ _) -> return ()
            _ -> assertFailure "Xception Test"
     , do exn <- try $ Client.testException prot "TException"
          case exn of
            Left (_ :: SomeException) -> return ()
            Right _ -> assertFailure "TException Test"
     , do exn <- try $ Client.testException prot "success"
          case exn of
            Left (_ :: SomeException) -> assertFailure "Exception Succes Test"
            Right _ -> return ()

     -- Multi Exception Tests
     , do exn <- try $ Client.testMultiException prot "Xception" "test 1"
          case exn of
            Left (Xception _ _) -> return ()
            _ -> assertFailure "Multi Exception 1"
     , do exn <- try $ Client.testMultiException prot "Xception2" "test 2"
          case exn of
            Left (Xception2 _ _) -> return ()
            _ -> assertFailure "Multi Exception 2"
     , do exn <- try $ Client.testMultiException prot "success" "test 3"
          case exn of
            Left (_ :: SomeException) -> assertFailure "Multi Exception 3"
            Right _ -> return ()
     ]

main :: IO Counts
main = do
  cs <- sequence [ runTest BinaryProtocol
                 , runTest CompactProtocol
                 , runTest SimpleJSONProtocol
                 ]
  return Counts{ cases    = total cases cs
               , tried    = total tried cs
               , errors   = total errors cs
               , failures = total failures cs
               }
  where
    runTest :: Protocol p => (Handle -> p Handle) -> IO Counts
    runTest prot = do
      port <- getFreePort
      let conn :: (String, PortID) = ("localhost", port) in
        bracket (listenOn port) N.sClose $ \socket ->
        bracket (forkIO $ runServer prot socket) killThread $ const $
        bracket (hOpen conn) tClose (runTestTT . runClient . prot)
    total a = sum . map a

    getFreePort :: IO PortID
    getFreePort =
      -- Attach to any socket then free it. The port should hopefully be open
      -- afterward, but it's possible that it's (1) not cleaned up fast enough
      -- or (2) another process grabs it immediately after it's released but
      -- before we grab it again.
      bracket (socket AF_INET Stream defaultProtocol) NS.close $ \s -> do
        bind s (SockAddrInet aNY_PORT iNADDR_ANY)
        N.socketPort s
