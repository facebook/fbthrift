<?hh // strict
/**
 * Trait for Thrift Structs to call into the Serialization Helper
 */
trait ThriftSerializationTrait implements \IThriftStruct {
  public abstract function readLegacy(\TProtocol $protocol) : int;
  public abstract function writeLegacy(\TProtocol $protocol) : int;

  public function read(\TProtocol $protocol) : int {
    return $this->readLegacy($protocol);
  }

  public function write(\TProtocol $protocol) : int {
    return $this->writeLegacy($protocol);
  }
}
