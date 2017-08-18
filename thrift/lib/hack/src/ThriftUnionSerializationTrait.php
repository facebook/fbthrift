<?hh // strict
// Copyright 2004-present Facebook. All Rights Reserved.

/**
 * Trait for Thrift Unions to call into the Serialization Helper
 */
trait ThriftUnionSerializationTrait implements \IThriftStruct {
  public function read(\TProtocol $protocol) : int {
    return \ThriftSerializationHelper::readUnion(
      $protocol,
      $this,
      /* HH_IGNORE_ERROR[4053] every thrift union has a _type field */
      $this->_type
    );
  }

  public function write(\TProtocol $protocol) : int {
    // Writing Structs and Unions is the same procedure.
    return \ThriftSerializationHelper::writeStruct($protocol, $this);
  }
}
