<?php
/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

if (!isset($GEN_DIR)) {
  $GEN_DIR = 'gen-php';
}
if (!isset($MODE)) {
  $MODE = 'normal';
}

/** Set the Thrift root */
$GLOBALS['THRIFT_ROOT'] = '../../lib/php/src';

/** Include the Thrift required libraries */
require_once $GLOBALS['THRIFT_ROOT'].'/Thrift.php';
require_once $GLOBALS['THRIFT_ROOT'].'/protocol/TBinaryProtocol.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TMemoryBuffer.php';

require_once $GEN_DIR.'/OptionalRequiredTest_types.php';

/////////////////////////////////
// SETTING THE VALUE
/////////////////////////////////

// Default gets written (when set)
$w = new Tricky1();
$r = new Tricky1();

$w->im_default = 10;
write_to_read($w, $r);
assert_equals(10, $r->im_default);

// Optional gets written (when set)
$w = new Tricky2();
$r = new Tricky2();

$w->im_optional = 10;
write_to_read($w, $r);
assert_equals(10, $r->im_optional);

// Require gets written (when set)
$w = new Tricky3();
$r = new Tricky3();

$w->im_required = 10;
write_to_read($w, $r);
assert_equals(10, $r->im_required);

/////////////////////////////////
// SETTING THE VALUE TO NULL
/////////////////////////////////

// Default gets written (when set  TO NULL)
$w = new Tricky1();
$r = new Tricky1();

$w->im_default = NULL;
write_to_read($w, $r);
assert_equals(NULL, $r->im_default);

// Optional gets written (when set TO NULL)
$w = new Tricky2();
$r = new Tricky2();

$w->im_optional = NULL;
write_to_read($w, $r);
assert_true(!isset($r->im_optional));

// Require gets written (when set TO NULL)
$w = new Tricky3();
$r = new Tricky3();

$w->im_required = NULL;

try {
  write_to_read($w, $r);
  assert_true(false, "Expected exception because field is required");
} catch (Exception $ex) {  }
assert_true(!isset($r->im_required));

/////////////////////////////////
// WITHOUT SETTING THE VALUE
/////////////////////////////////

// Default gets written (when set)
$w = new Tricky1();
$r = new Tricky1();

write_to_read($w, $r);
assert_true(!isset($r->im_default));

// Optional gets written (when set)
$w = new Tricky2();
$r = new Tricky2();

write_to_read($w, $r);
assert_true(!isset($r->im_optional));

// Require gets written (when set)
$w = new Tricky3();
$r = new Tricky3();

try {
  write_to_read($w, $r);
  assert_true(false, "Expected exception because field is required");
} catch (Exception $ex) {  }
assert_true(!isset($r->im_required));

/////////////////////////////////
// Mixing Default & Optional & Required
/////////////////////////////////

// Default <-> Optional
$w = new Tricky1();
$r = new Tricky2();

$w->im_default = 0;
$r->im_optional = 10;

write_to_read($w, $r);
assert_equals(0, $r->im_optional);
assert_equals(0, $w->im_default);

write_to_read($r, $w);
assert_equals(0, $r->im_optional);
assert_equals(0, $w->im_default);

// Default <-> Required
$w = new Tricky1();
$r = new Tricky3();

$w->im_default = 0;
$r->im_required = 10;

write_to_read($w, $r);
assert_equals(0, $r->im_required);
assert_equals(0, $w->im_default);

write_to_read($r, $w);
assert_equals(0, $r->im_required);
assert_equals(0, $w->im_default);

echo "Tests passed!\n";

function write_to_read($write_struct, $read_struct) {
  $protocol = new TBinaryProtocol(new TMemoryBuffer());
  $write_struct->write($protocol);
  $read_struct->read($protocol);
}

function assert_true($bool) {
  if (!$bool) {
    throw new Exception("Assertion is false");
  }
}

function assert_equals($expected, $actual) {
  if ($expected != $actual) {
    throw new Exception("Variable was expected contain the value '$expected'" .
      " the actual value was '$actual'.");
  }
}

?>
