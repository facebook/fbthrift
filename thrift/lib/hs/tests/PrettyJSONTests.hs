{-# LANGUAGE OverloadedStrings #-}
module PrettyJSONTests where

import Data.IORef
import Test.HUnit
import Thrift.Protocol.PrettyJSON
import Thrift.Transport.Empty
import ThriftTest_Types
import qualified Data.ByteString.Lazy as BSL
import qualified Data.HashMap.Strict as Map
import qualified Data.Vector as Vector

import Util

tests :: Test
tests = TestList
  [ TestLabel "Pretty printing test" $ TestCase $ do
    ref <- newIORef ""
    let t = TestTransport ref
    let struct = Insanity
          { insanity_userMap = Map.fromList [(TWO, 4), (SIX, 7)]
          , insanity_xtructs = Vector.fromList
            [ Xtruct "1" 2 3 4
            , Xtruct "5" 6 7 8
            ]
          }
    write_Insanity (PrettyJSONProtocol 3 t) struct
    result <- readIORef ref
    let expected = BSL.intercalate "\n"
          [ "{"
          , "   \"userMap\": {"
          , "      \"6\": 7,"
          , "      \"2\": 4"
          , "   },"
          , "   \"xtructs\": ["
          , "      {"
          , "         \"string_thing\": \"1\","
          , "         \"byte_thing\": 2,"
          , "         \"i32_thing\": 3,"
          , "         \"i64_thing\": 4"
          , "      },"
          , "      {"
          , "         \"string_thing\": \"5\","
          , "         \"byte_thing\": 6,"
          , "         \"i32_thing\": 7,"
          , "         \"i64_thing\": 8"
          , "      }"
          , "   ]"
          , "}"
          ]

    assertEqual "Insanity" expected result
  , TestLabel "Escaped JSON test" $ TestCase $ do
    let json = BSL.intercalate "\n"
          [ "{"
          , "  \"string_thing\": \"\\u0074\\u0065\\u0073\\u0074999\","
          , "  \"byte_thing\": 0,"
          , "  \"i32_thing\": 0,"
          , "  \"i64_thing\": 0"
          , "}"
          ]
        expected = Xtruct "test999" 0 0 0
        result = decode_Xtruct (PrettyJSONProtocol 2 EmptyTransport) json
    assertEqual "parse escaped" expected result
  ]

main :: IO Counts
main = runTestTT tests
