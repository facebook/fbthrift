<?hh
// Copyright 2004-present Facebook. All Rights Reserved.

/**
 * Trait for Thrift Unions to call into the Serialization Helper
 */
trait ThriftUnionSerializationTrait implements \IThriftStruct {

  public function read(\TProtocol $protocol): int {
    /* HH_IGNORE_ERROR[4053] every thrift union has a _type field */
    $type = $this->_type;
    $ret = \ThriftSerializationHelper::readUnion($protocol, $this, inout $type);
    /* HH_IGNORE_ERROR[4053] every thrift union has a _type field */
    $this->_type = $type;
    return $ret;
  }

  public function write(\TProtocol $protocol): int {
    // Writing Structs and Unions is the same procedure.
    return \ThriftSerializationHelper::writeStruct($protocol, $this);
  }
}
