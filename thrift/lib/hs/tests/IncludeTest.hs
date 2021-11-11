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

module IncludeTest where

import Test.QuickCheck

import IncludeTest_Types
import IncludedTest_Types

import Util

propToFoo = foo == roundTrip foo
  where foo = Foo { foo_Bar = 5, foo_Baz = 10 }
        roundTrip = to_Foo . from_Foo

propDefaultFoo = default_Foo { foo_Baz = 1 } == bar_baz default_Bar

main :: IO ()
main = aggregateResults $ fmap quickCheckResult [propToFoo, propDefaultFoo]
