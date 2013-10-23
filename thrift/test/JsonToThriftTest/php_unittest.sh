#!/bin/sh
#
THRIFT_ROOT=`readlink -m thrift/lib/php/src`
GEN_ROOT=`readlink -m _bin/thrift/test/JsonToThriftTest/thrift_test_json-php.thrift-gen`
PACKAGE_ROOT=`readlink -m _bin/thrift/test/JsonToThriftTest/packages`
HERE=`pwd -L`

echo "copy necessary files"

mkdir -p ${PACKAGE_ROOT} #2>/dev/null
mkdir -p ${PACKAGE_ROOT}/myBinaryStruct #2>/dev/null 
mkdir -p ${PACKAGE_ROOT}/myBoolStruct #2>/dev/null
mkdir -p ${PACKAGE_ROOT}/myByteStruct #2>/dev/null
mkdir -p ${PACKAGE_ROOT}/myComplexStruct #2>/dev/null
mkdir -p ${PACKAGE_ROOT}/myDoubleStruct #2>/dev/null
mkdir -p ${PACKAGE_ROOT}/myI16Struct #2>/dev/null
mkdir -p ${PACKAGE_ROOT}/myI32Struct #2>/dev/null
mkdir -p ${PACKAGE_ROOT}/myPHPMapStruct #2>/dev/null
mkdir -p ${PACKAGE_ROOT}/myMixedStruct #2>/dev/null
mkdir -p ${PACKAGE_ROOT}/mySetStruct #2>/dev/null
mkdir -p ${PACKAGE_ROOT}/mySimpleStruct #2>/dev/null
mkdir -p ${PACKAGE_ROOT}/myStringStruct #2>/dev/null
mkdir -p ${PACKAGE_ROOT}/config #2>/dev/null

# Copy all thrift php library to $GEN_ROOT
cp -R -f ${THRIFT_ROOT}/. ${GEN_ROOT}/..
cp -f ${GEN_ROOT}/myBinaryStruct/gen-php/myBinaryStruct_types.php ${PACKAGE_ROOT}/myBinaryStruct/myBinaryStruct_types.php
cp -f ${GEN_ROOT}/myBoolStruct/gen-php/myBoolStruct_types.php ${PACKAGE_ROOT}/myBoolStruct/myBoolStruct_types.php
cp -f ${GEN_ROOT}/myByteStruct/gen-php/myByteStruct_types.php ${PACKAGE_ROOT}/myByteStruct/myByteStruct_types.php
cp -f ${GEN_ROOT}/myComplexStruct/gen-php/myComplexStruct_types.php ${PACKAGE_ROOT}/myComplexStruct/myComplexStruct_types.php
cp -f ${GEN_ROOT}/myDoubleStruct/gen-php/myDoubleStruct_types.php ${PACKAGE_ROOT}/myDoubleStruct/myDoubleStruct_types.php
cp -f ${GEN_ROOT}/myI16Struct/gen-php/myI16Struct_types.php ${PACKAGE_ROOT}/myI16Struct/myI16Struct_types.php
cp -f ${GEN_ROOT}/myI32Struct/gen-php/myI32Struct_types.php ${PACKAGE_ROOT}/myI32Struct/myI32Struct_types.php
cp -f ${GEN_ROOT}/myPHPMapStruct/gen-php/myPHPMapStruct_types.php ${PACKAGE_ROOT}/myPHPMapStruct/myPHPMapStruct_types.php
cp -f ${GEN_ROOT}/myMixedStruct/gen-php/myMixedStruct_types.php ${PACKAGE_ROOT}/myMixedStruct/myMixedStruct_types.php
cp -f ${GEN_ROOT}/mySetStruct/gen-php/mySetStruct_types.php ${PACKAGE_ROOT}/mySetStruct/mySetStruct_types.php
cp -f ${GEN_ROOT}/mySimpleStruct/gen-php/mySimpleStruct_types.php ${PACKAGE_ROOT}/mySimpleStruct/mySimpleStruct_types.php
cp -f ${GEN_ROOT}/myStringStruct/gen-php/myStringStruct_types.php ${PACKAGE_ROOT}/myStringStruct/myStringStruct_types.php
cp -f ${GEN_ROOT}/config/gen-php/config_types.php ${PACKAGE_ROOT}/config/config_types.php

echo Testing...

/home/engshare/svnroot/tfb/trunk/www/scripts/bin/hphpi ${HERE}/thrift/test/JsonToThriftTest/SimpleJSONToThriftTest.php
exit_code=$?

rm -rf ${PACKAGE_ROOT}/myBinaryStruct
rm -rf ${PACKAGE_ROOT}/myBoolStruct
rm -rf ${PACKAGE_ROOT}/myByteStruct
rm -rf ${PACKAGE_ROOT}/myComplexStruct
rm -rf ${PACKAGE_ROOT}/myDoubleStruct
rm -rf ${PACKAGE_ROOT}/myI16Struct
rm -rf ${PACKAGE_ROOT}/myI32Struct
rm -rf ${PACKAGE_ROOT}/myPHPMapStruct
rm -rf ${PACKAGE_ROOT}/myMixedStruct
rm -rf ${PACKAGE_ROOT}/mySetStruct
rm -rf ${PACKAGE_ROOT}/mySimpleStruct
rm -rf ${PACKAGE_ROOT}/myStringStruct
rm -rf ${PACKAGE_ROOT}/config

exit $exit_code;
