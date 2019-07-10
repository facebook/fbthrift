<?hh

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
 * Trait for Thrift Structs to call into the Serialization Helper
 */
trait ThriftSerializationTrait implements IThriftStruct {

  <<__Rx, __AtMostRxAsArgs, __Mutable>>
  public function read(
    <<__OnlyRxIfImpl(IRxTProtocol::class), __Mutable>>
    TProtocol $protocol,
  ): int {
    return ThriftSerializationHelper::readStruct($protocol, $this);
  }

  public function write(TProtocol $protocol): int {
    return ThriftSerializationHelper::writeStruct($protocol, $this);
  }
}
