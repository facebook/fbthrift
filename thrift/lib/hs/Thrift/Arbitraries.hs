{-# OPTIONS_GHC -fno-warn-orphans #-}

module Thrift.Arbitraries where

import Data.Bits()

import Test.QuickCheck.Arbitrary
import Test.QuickCheck.Gen

import qualified Data.Map as Map
import qualified Data.Set as Set
import qualified Data.Vector as Vector
import qualified Data.Text.Lazy as Text
import qualified Data.HashMap.Strict as HMap
import Data.Hashable (Hashable)

import Data.ByteString.Lazy (ByteString)
import qualified Data.ByteString.Lazy.Char8 as BS

-- String has an Arbitrary instance already
-- Bool has an Arbitrary instance already
-- A Thrift 'list' is a [].
-- [] has an Arbitrary instance already

instance Arbitrary ByteString
  where arbitrary = do lst <- (arbitrary :: Gen String)
                       return $ BS.pack lst

instance (Ord k, Arbitrary k, Arbitrary v) => Arbitrary (Map.Map k v)
   where arbitrary = do lst <- arbitrary
                        return $ Map.fromList lst

instance (Ord k, Arbitrary k) => Arbitrary (Set.Set k)
    where arbitrary = do lst <- arbitrary
                         return $ Set.fromList lst

instance (Arbitrary k) => Arbitrary (Vector.Vector k)
    where arbitrary = do lst <- arbitrary
                         return $ Vector.fromList lst

instance Arbitrary Text.Text
    where arbitrary = do str <- arbitrary
                         return $ Text.pack str

instance (Ord k, Hashable k, Arbitrary k, Arbitrary v) => Arbitrary (HMap.HashMap k v)
   where arbitrary = do lst <- arbitrary
                        return $ HMap.fromList lst

{-
   To handle Thrift 'enum' we would ideally use something like:

instance (Enum a, Bounded a) => Arbitrary a
    where arbitrary = elements (enumFromTo minBound maxBound)

Unfortunately this doesn't play nicely with the type system.
Instead we'll generate an arbitrary instance along with the code.
-}

{-
    There might be some way to introspect on the Haskell structure of a
    Thrift 'struct' or 'exception' but generating the code directly is simpler.
-}

