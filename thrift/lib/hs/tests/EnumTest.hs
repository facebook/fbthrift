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
