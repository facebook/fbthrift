<?hh // partial

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
/**
 * Can be called with standard Exception constructor (message, code) or with
 * Thrift Base object constructor (spec, vals).
 *
 * @param mixed $p1 Message (string) or type-spec (array)
 * @param mixed $p2 Code (integer) or values (array)
 */
class TException extends Exception {

  public string $message = '';
  public int $code = 0;

  <<__Rx>>
  public function __construct($p1 = null, $p2 = 0) {
    if (is_array($p1) && is_array($p2)) {
      $spec = $p1;
      $vals = $p2;
      foreach ($spec as $fid => $fspec) {
        $var = $fspec['var'];
        if (isset($vals[$var])) {
          /* HH_FIXME[2011] dynamic method is allowed on non dynamic types */
          $this->$var = $vals[$var];
        }
      }
      parent::__construct();
    } else {
      /* HH_FIXME[4281] error revealed by improved type refinements */
      parent::__construct((string) $p1, $p2);
    }
  }
}
