<?php

/**
 * This unit test first copy the thrift php source file
 * from thrift/lib/php/src to _bin/thrift/test/JsonToThriftTest
 */

$FBCODE_DIR = './';
$HERE = $FBCODE_DIR.'thrift/test/JsonToThriftTest';

// Temporarily change to gen-php
$GLOBALS['THRIFT_ROOT'] =
  $FBCODE_DIR.'_bin/thrift/test/JsonToThriftTest';

$PACKAGE_DIR = $GLOBALS['THRIFT_ROOT'].'/packages';

require_once $PACKAGE_DIR.'/myBinaryStruct/myBinaryStruct_types.php';
require_once $PACKAGE_DIR.'/myBoolStruct/myBoolStruct_types.php';
require_once $PACKAGE_DIR.'/myByteStruct/myByteStruct_types.php';
require_once $PACKAGE_DIR.'/myComplexStruct/myComplexStruct_types.php';
require_once $PACKAGE_DIR.'/myDoubleStruct/myDoubleStruct_types.php';
require_once $PACKAGE_DIR.'/myI16Struct/myI16Struct_types.php';
require_once $PACKAGE_DIR.'/myI32Struct/myI32Struct_types.php';
require_once $PACKAGE_DIR.'/myPHPMapStruct/myPHPMapStruct_types.php';
require_once $PACKAGE_DIR.'/myMixedStruct/myMixedStruct_types.php';
require_once $PACKAGE_DIR.'/mySetStruct/mySetStruct_types.php';
require_once $PACKAGE_DIR.'/mySimpleStruct/mySimpleStruct_types.php';
require_once $PACKAGE_DIR.'/myStringStruct/myStringStruct_types.php';
require_once $PACKAGE_DIR.'/config/config_types.php';

/** Include the Thrift base */
require_once $GLOBALS['THRIFT_ROOT'].'/Thrift.php';

/** Include the binary protocol */
require_once $GLOBALS['THRIFT_ROOT'].'/protocol/TBinaryProtocol.php';

/** Include the socket layer */
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TSocketPool.php';

/** Include the socket layer */
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TBufferedTransport.php';


/**
 * Util Functions
 */
function assert_array_equals($a, $b) {
  $res = !array_diff($a, $b) && !array_diff($b, $a);
  if (!$res) {
    print "expected value: \n";
    print_r($b);
    print "\nactual value: \n";
    print_r($a);
    fail("The actual value differs from the expected value.");
  }
}

function assert_true($bool) {
  if (!$bool) {
    fail("Assertion fails or produces unintended exceptions.");
  }
}

function assert_equals($expected, $actual) {
  if ($expected != $actual) {
    print "expected value: \n";
    print_r($expected);
    print "\nactual value: \n";
    if ($actual == null) {
      print "null";
    } else {
      print_r($actual);
    }
    fail("The actual value differs from the expected value.");
  }
}

function fail($message) {
  throw new Exception($message);
}

function test_exception($thrift_obj, $json_str) {
  $flag = false;
  try {
    $thrift_obj->readFromJson($json_str);
    $flag = true;
  } catch (Exception $e) {
  }
  if ($flag) {
    fail("Fail to throw an exception for undesired input.");
  }
}

/**
 * Test Setup
 */
$binaryStruct = new myBinaryStruct(array());
$boolStruct = new myBoolStruct(array());
$byteStruct = new myByteStruct(array());
$complexStruct = new myComplexStruct(array());
$doubleStruct = new myDoubleStruct(array());
$i16Struct = new myI16Struct(array());
$i32Struct = new myI32Struct(array());
$mapStruct = new myMapStruct(array());
$mixedStruct = new myMixedStruct(array());
$setStruct = new mySetStruct(array());
$simpleStruct = new mySimpleStruct(array());
$stringStruct = new myStringStruct(array());

/**
 * Temp values used for testing
 */
$complexArray = array(314, 15, 9, 26535);
$complexMapElem1 = new mySimpleStruct(array('c' => 1));
$complexMapElem2 = new mySimpleStruct(array('a' => false, 'b' => -4,
                                            'c' => 5));
$byteMap = array(1 =>'one', 2 => 'two');
$enumMap = array(Gender::MALE => 'male', Gender::FEMALE => 'female');
$boolMap = array(false => 'False', true => 'True');
$stringMap = array('a' => 'A', 'b' => 'B');


/**
 * Test Binary
 */
$binaryJson = "{\n \"a\": \"xyzzy\"\n}";
$binaryStruct->readFromJson($binaryJson);
assert_equals($binaryStruct->a, "xyzzy");

/**
 * Test Bool
 */
$boolJson1 = "{\n  \"a\": true\n}";
$boolStruct->readFromJson($boolJson1);
assert_equals($boolStruct->a, true);

$boolJson2 = "{\n  \"a\": false\n}";
$boolStruct->readFromJson($boolJson2);
assert_equals($boolStruct->a, false);

/**
 * Test Byte
 * One case contains a number larger than a byte,
 * should throw an exception
 */
$byteJson1 = "{\n  \"a\": 101\n}";
$byteStruct->readFromJson($byteJson1);
assert_equals($byteStruct->a, 101);

$byteJson2 = "{\n  \"a\": 256\n}";
test_exception($byteStruct, $byteJson2);

/**
 * Test Complex Structure
 */
$complexJson1 = "{\n  \"a\": {\n    \"a\": true,\n    \"b\": 92,\n    \"c\":
  902,\n    \"d\": 65536,\n    \"e\": 123456789,\n    \"f\": 3.1415,\n    \"g\":
  \"Whan that Aprille\"\n  },\n  \"b\": [\n    314, 15, 9, 26535\n  ],\n  \"c\":
  {\n    \"qwerty\": {\n        \"c\": 1\n    },\n    \"slippy\": {\n
  \"a\": false,\n        \"b\": -4,\n        \"c\": 5\n    }\n  },\n  \"e\":
  3,\n  \"x\": {\n    \"message\": \"test\"\n  }\n}";
$complexStruct->readFromJson($complexJson1);
assert_equals($complexStruct->a->a, true);
assert_equals($complexStruct->a->b, 92);
assert_equals($complexStruct->a->c, 902);
assert_equals($complexStruct->a->e, 123456789);
assert_equals($complexStruct->a->d, 65536);
assert_equals($complexStruct->a->f, 3.1415);
assert_equals($complexStruct->a->g, 'Whan that Aprille');
assert_array_equals($complexStruct->b, $complexArray);
assert_equals($complexStruct->c['qwerty'], $complexMapElem1);
assert_equals($complexStruct->c['slippy'], $complexMapElem2);
assert_equals($complexStruct->x->message, 'test');
assert_equals($complexStruct->e, 3);

$complexJson2 = "{\n}";
$complexStruct = new myComplexStruct(array());
$complexStruct->readFromJson($complexJson2);
assert_equals($complexStruct->a, null);
assert_equals($complexStruct->b, null);
assert_equals($complexStruct->c, null);
assert_equals($complexStruct->x, null);
assert_equals($complexStruct->e, null);

/**
 * Test Double
 */
$doubleJson1 = "{\n  \"a\": -2.192\n}";
$doubleStruct->readFromJson($doubleJson1);
assert_equals($doubleStruct->a, -2.192);

/**
 * Test Int16 and Int 32
 */
$i16Json1 = "{\n  \"a\": 4567\n}";
$i16Struct->readFromJson($i16Json1);
assert_equals($i16Struct->a, 4567);

$i16Json2 = "{\n  \"a\": 68414056839\n}";
test_exception($i16Struct, $i16Json2);

$i32Json1 = "{\n  \"a\": 12131415\n}";
$i32Struct->readFromJson($i32Json1);
assert_equals($i32Struct->a, 12131415);

$i32Json2 = "{\n  \"a\": 4503599627295930\n}";
test_exception($i32Struct, $i32Json2);

/**
 * Test Map
 * Currently only support integer and string as map key
 */
$mapJson = "{\n  \"stringMap\": {\n    \"a\": \"A\",\n    \"b\": \"B\"\n  },\n
  \"boolMap\": {\n    \"false\": \"False\",\n    \"true\": \"True\"\n  },\n
  \"byteMap\": {\n   \"1\": \"one\",\n    \"2\": \"two\"\n  },\n  \"doubleMap\":
  {\n    \"0.2\": \"0.two\",\n    \"0.1\": \"0.one\"\n  },\n  \"enumMap\": {\n
  \"1\": \"male\",\n    \"2\": \"female\"\n  }\n}";
$mapStruct->readFromJson($mapJson);
assert_array_equals($mapStruct->stringMap, $stringMap);
assert_array_equals($mapStruct->boolMap, $boolMap);
assert_array_equals($mapStruct->byteMap, $byteMap);
assert_array_equals($mapStruct->enumMap, $enumMap);


/**
 * Test mixed structure
 */
$mixedJson ="{\n  \"a\": [\n    \n  ],\n  \"b\": [\n    {\n      \"a\": 5\n
  }\n  ],\n  \"c\": {\n   \"fire\": -191,\n    \"flame\": -8\n  },\n  \"d\": {\n
  \n  },\n  \"e\": [\n    1, 2, 3, 4\n  ]\n}";
$mixedStruct->readFromJson($mixedJson);
assert_array_equals($mixedStruct->a, array());
assert_array_equals($mixedStruct->c, array('fire' => -191, 'flame' => -8));
$tSuperStruct = new mySuperSimpleStruct(array('a' => 5));
$tArr = array(0 => $tSuperStruct);
assert_true($mixedStruct->b == $tArr);
assert_array_equals($mixedStruct->d, array());
assert_array_equals($mixedStruct->e, array(1 => true, 2 => true,
                                           3 => true, 4 => true));
/**
 * Test Set
 */
$setJson1 = "{\n  \"a\": [\n    8, 16, 4, 15\n  ]\n}";
$setStruct->readFromJson($setJson1);
assert_array_equals($setStruct->a, array(4 => true, 8 => true, 15 => true,
      16 => true));

$setJson2 = "{\n  \"a\": [\n    \n  ]\n}";
$setStruct->readFromJson($setJson2);
assert_array_equals($setStruct->a, array());

$setJson3 = "{\n  \"a\": [\n    1, 2, 1099511627775\n  ]\n}";
test_exception($setStruct, $setJson3);

/**
 * Test for simple structure
 */
$simpleJson1 = "{\n  \"a\": false,\n  \"b\": 87,\n  \"c\": 7880,\n  \"d\":
  -7880,\n  \"e\":-1,\n  \"f\": -0.1,\n  \"g\": \"T-bone\"\n}";
$simpleStruct->readFromJson($simpleJson1);
assert_equals($simpleStruct->a, false);
assert_equals($simpleStruct->b, 87);
assert_equals($simpleStruct->c, 7880);
assert_equals($simpleStruct->d, -7880);
assert_equals($simpleStruct->e, -1);
assert_equals($simpleStruct->f, -0.1);
assert_equals($simpleStruct->g, "T-bone");

$simpleJson2 = "{\n  \"c\": 9\n}";
$simpleStruct = new mySimpleStruct(array());
$simpleStruct->readFromJson($simpleJson2);
assert_equals($simpleStruct->c, 9);
assert_equals($simpleStruct->f, null);

$simpleJson3 = "{\n}";
$simpleStruct = new mySimpleStruct(array());
test_exception($simpleStruct, $simpleJson3);

/**
 * Test String (empty, null, and normal)
 */
$strJson1 = "{\n  \"a\": \"\"\n}";
$stringStruct->readFromJson($strJson1);
assert_equals($stringStruct->a, "");
$strJson2 = "{\n}";
$stringStruct->readFromJson($strJson2);
assert_equals($stringStruct->a, null);
$strJson3 = "{\n  \"a\": \"foobar\"\n}";
$stringStruct->readFromJson($strJson3);
assert_equals($stringStruct->a, "foobar");

/**
 * Integration Test
 * This config file is from configerator/materialize_configs/ffuuu
 * It should not product any exceptions
 */
try {
  $config = new Config(array());
  $config->readFromJson(file_get_contents($HERE.'/config.json'));
} catch (Exception $e) {
  print_r($e);
  fail("Fail intregation test!");
}

/**
 * Test invalid JSON string
 */
$invalidJson1 = "{ 'a': 'baz' }";
test_exception($stringStruct, $invalidJson1);
$invalidJson2 = '{ a : "baz" }';
test_exception($stringStruct, $invalidJson2);
$invalidJson3 = '{ "bar" : "baz", }';
test_exception($stringStruct, $invalidJson3);
$invalidJson4 = 'foo: bar';
test_exception($stringStruct, $invalidJson4);
// Restore THRIFT_ROOT
$GLOBALS['THRIFT_ROOT'] = $TMP_THRIFT_ROOT;
print "Success!";
return 0;
?>

