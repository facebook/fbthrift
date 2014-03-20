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
{-# LANGUAGE ScopedTypeVariables #-}

module Client where

import Control.Exception as CE
import qualified Data.HashMap.Strict as Map
import qualified Data.HashSet as Set
import qualified Data.Vector as Vec
import Network

import Thrift
import Thrift.Protocol.Binary
import Thrift.Transport.Handle

import ThriftTest_Client
import ThriftTest_Types

t :: (HostName, PortID)
t = ("127.0.0.1", PortNumber 9090)

main :: IO ()
main = do
  to <- hOpen t
  let p =  BinaryProtocol to
  let ps = (p,p)
  print =<< testString ps "bya"
  print =<< testByte ps 8
  print =<< testByte ps (-8)
  print =<< testI32 ps 32
  print =<< testI32 ps (-32)
  print =<< testI64 ps 64
  print =<< testI64 ps (-64)
  print =<< testFloat ps (exp 1)
  print =<< testFloat ps (- exp 1)
  print =<< testDouble ps pi
  print =<< testDouble ps (- pi)
  print =<< testMap ps (Map.fromList [(1,1),(2,2),(3,3)])
  print =<< testList ps (Vec.fromList [1,2,3,4,5])
  print =<< testSet ps (Set.fromList [1,2,3,4,5])
  print =<< testStruct ps (Xtruct (Just "hi") (Just 4) (Just 5) Nothing)
  CE.catch (testException ps "e" >> putStrLn "bad")
           (\e -> print (e :: Xception))
  CE.catch (testMultiException ps "e" "e2" >> putStrLn "ok")
           (\e -> print (e :: Xception))
  CE.catch (CE.catch (testMultiException ps "e" "e2">> putStrLn "bad")
                     (\e -> print (e :: Xception2)))
           (\(_ :: SomeException) -> putStrLn "ok")
  tClose to

