<?hh // strict

/**
* Copyright (c) 2006- Facebook
* Distributed under the Thrift Software License
*
* See accompanying file LICENSE or visit the Thrift site at:
* http://developers.facebook.com/thrift/
*
* @package thrift
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
  ): darray<Tk, Tv> {
    $result = darray[];
    foreach ($traversable as $key => $value) {
      $result[HH\array_key_cast($key)] = $value;
    }
    /* HH_IGNORE_ERROR[4110] maintain lie */
    return $result;
  }
}
