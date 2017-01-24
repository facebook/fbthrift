<?hh // strict

abstract final class ThriftUtil {
  public static function mapDict<Tk, Tv1, Tv2>(
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
}
