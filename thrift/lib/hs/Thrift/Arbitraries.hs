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

{-# LANGUAGE CPP #-}
{-# OPTIONS_GHC -fno-warn-orphans #-}

module Thrift.Arbitraries where

import Data.Bits()

import Test.QuickCheck.Arbitrary

#if __GLASGOW_HASKELL__ < 710
import Control.Applicative ((<$>))
#endif
import qualified Data.Vector as Vector
import qualified Data.Text as Text
import qualified Data.Text.Lazy as LT
import qualified Data.HashSet as HSet
import qualified Data.HashMap.Strict as HMap
import Data.Hashable (Hashable)

import Data.ByteString.Lazy (ByteString)
import qualified Data.ByteString.Lazy as BS

-- String has an Arbitrary instance already
-- Bool has an Arbitrary instance already
-- A Thrift 'list' is a Vector.

instance Arbitrary ByteString where
  arbitrary = BS.pack . filter (/= 0) <$> arbitrary

instance (Arbitrary k) => Arbitrary (Vector.Vector k) where
  arbitrary = Vector.fromList <$> arbitrary

instance Arbitrary Text.Text where
  arbitrary = Text.pack . filter (/= '\0') <$> arbitrary

instance Arbitrary LT.Text where
  arbitrary = LT.pack . filter (/= '\0') <$> arbitrary

instance (Eq k, Hashable k, Arbitrary k) => Arbitrary (HSet.HashSet k) where
  arbitrary = HSet.fromList <$> arbitrary

instance (Eq k, Hashable k, Arbitrary k, Arbitrary v) =>
    Arbitrary (HMap.HashMap k v) where
  arbitrary = HMap.fromList <$> arbitrary

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
