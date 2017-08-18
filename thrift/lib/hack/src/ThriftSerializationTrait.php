<?hh // strict
// Copyright 2004-present Facebook. All Rights Reserved.

/**
 * Trait for Thrift Structs to call into the Serialization Helper
 */
trait ThriftSerializationTrait implements \IThriftStruct {
  public function read(\TProtocol $protocol) : int {
    return \ThriftSerializationHelper::readStruct($protocol, $this);
  }

  public function write(\TProtocol $protocol) : int {
    return \ThriftSerializationHelper::writeStruct($protocol, $this);
  }
}
