{-
  Copyright (c) Facebook, Inc. and its affiliates.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
-}

module EnumTest where

import ThriftTest_Types

import Test.HUnit

main :: IO Counts
main = runTestTT $ TestList

  [ TestLabel "succ" $ TestCase $ do
    assertEqual "" TWO $ succ ONE
    assertEqual "" FIVE $ succ THREE

  , TestLabel "pred" $ TestCase $ do
    assertEqual "" ONE $ pred TWO
    assertEqual "" THREE $ pred FIVE

  , TestLabel ".." $ TestCase $ do
    assertEqual "" [ONE, TWO, THREE, FIVE, SIX, EIGHT] [minBound..]
    assertEqual "" [ONE, TWO, THREE, FIVE, SIX, EIGHT] [minBound..maxBound]

  ]
