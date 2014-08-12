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

{-# LANGUAGE OverloadedStrings #-}
module TestServer
       ( runServer
       ) where

import Control.Exception
import Network
import System.IO
import qualified Data.HashMap.Strict as Map
import qualified Data.Text.Lazy as Text

import ThriftTest
import ThriftTest_Iface
import ThriftTest_Types

import Thrift
import Thrift.Server

data TestHandler = TestHandler
instance ThriftTest_Iface TestHandler where
  testVoid _ = return ()
  testString _ = return
  testByte _ = return
  testI32 _ = return
  testI64 _ = return
  testDouble _ = return
  testStruct _ = return
  testNest _ = return
  testMap _ = return
  testStringMap _ = return
  testSet _ = return
  testList _ = return
  testEnum _ = return
  testTypedef _ = return
  testMapMap _ _ = return $ Map.fromList
    [ (-4, Map.fromList [ (-4, -4)
                        , (-3, -3)
                        , (-2, -2)
                        , (-1, -1)
                        ])
    , (4,  Map.fromList [ (1, 1)
                        , (2, 2)
                        , (3, 3)
                        , (4, 4)
                        ])
    ]
  testInsanity _ x = return $ Map.fromList
    [ (1, Map.fromList [ (TWO  , x)
                       , (THREE, x)
                       ])
    , (2, Map.fromList [ (SIX, default_Insanity)
                       ])
    ]
  testMulti _ byte i32 i64 _ _ _ = return Xtruct
    { xtruct_string_thing = Text.pack "Hello2"
    , xtruct_byte_thing   = byte
    , xtruct_i32_thing    = i32
    , xtruct_i64_thing    = i64
    }
  testException _ s = case s of
    "Xception"   -> throw $ Xception 1001 s
    "TException" -> throw ThriftException
    _ -> return ()

  testMultiException _ s1 s2 = case s1 of
    "Xception"   -> throw $ Xception 1001 "This is an Xception"
    "Xception2"  -> throw $ Xception2 2002 default_Xtruct
    "TException" -> throw ThriftException
    _ -> return default_Xtruct{ xtruct_string_thing = s2 }

  testOneway _ _ = return ()

runServer :: Protocol p => (Handle -> p Handle) -> Socket -> IO ()
runServer p = runThreadedServer (accepter p) TestHandler ThriftTest.process
  where
    accepter p s = do
      (h, _, _) <- accept s
      return (p h, p h)
