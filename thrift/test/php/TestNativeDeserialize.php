#!/bin/env php
<?php
// Intended to be called from fbcode/thrift/test/PhpSerializeTest.cpp
// Verifies that serialized data written by that program can be
// deserialized correctly.

$FBCODE_DIR=dirname(__FILE__).'/../../..';
if (!isset($GLOBALS['THRIFT_ROOT'])) {
  $GLOBALS['THRIFT_ROOT'] = $FBCODE_DIR.'/thrift/lib/php/src';
}
require_once $FBCODE_DIR.
              '/_bin/thrift/test/gen-php/PhpSerializeTest_types.php';

function handle_err($errno, $errstr, $errfile, $errline) {
  print "$errno : $errstr at line $errline in $errfile \n";
  exit(1);
}

function handle_excn($ex) {
  print_r($ex);
  print("\n");
  exit(1);
}

function verifySimpleData($d) {
  assert(intval($d->c) == ($d->a + 2));
}

function verifySecondLevel($s) {
  for($i = 0; $i < count($s->e); $i++) {
    $key = $s->d[$i];
    $rnd = $s->e[$i];
    verifySimpleData($s->a[$key]);
    verifySimpleData($s->c[$i]);
    assert($s->b[$rnd] == $key);
    assert($s->a[$key]->a == $rnd);
  }
}

set_error_handler('handle_err');
set_exception_handler('handle_excn');

if (count($argv) < 2) {
  exit(1);
}

$serialized = file_get_contents($argv[1]);
$test_obj = unserialize($serialized);
verifySimpleData($test_obj->c);
foreach ($test_obj->b as $key) {
  verifySecondLevel($test_obj->a[$key]);
}

exit(0);

?>
