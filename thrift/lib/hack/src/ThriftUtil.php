<?hh

/*
 * Copyright 2006-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
abstract final class ThriftUtil {

  public static function mapDict<Tk as arraykey, Tv1, Tv2>(
    KeyedTraversable<Tk, Tv1> $traversable,
    (function(Tv1): Tv2) $value_func,
  ): dict<Tk, Tv2> {
    $result = dict[];
    foreach ($traversable as $key => $value) {
      $result[$key] = $value_func($value);
    }
    return $result;
  }

  public static function mapVec<Tv1, Tv2>(
    Traversable<Tv1> $traversable,
    (function(Tv1): Tv2) $value_func,
  ): vec<Tv2> {
    $result = vec[];
    foreach ($traversable as $value) {
      $result[] = $value_func($value);
    }
    return $result;
  }

  public static function mapKeyset<Tv1, Tv2 as arraykey>(
    Traversable<Tv1> $traversable,
    (function(Tv1): Tv2) $value_func,
  ): keyset<Tv2> {
    $result = keyset[];
    foreach ($traversable as $value) {
      $result[] = $value_func($value);
    }
    return $result;
  }

  public static function toDArray<Tk, Tv>(
    KeyedTraversable<Tk, Tv> $traversable,
  /* HH_FIXME[4319] Revealed by enforcing darray type declarations to have arraykey key types */
  ): darray<Tk, Tv> {
    $result = darray[];
    foreach ($traversable as $key => $value) {
      $result[HH\array_key_cast($key)] = $value;
    }
    /* HH_IGNORE_ERROR[4110] maintain lie */
    return $result;
  }
}
