/**
 * Autogenerated by Thrift for thrift/compiler/test/fixtures/frozen-struct/src/include2.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated @nocommit
 */
#include "thrift/compiler/test/fixtures/frozen-struct/gen-cpp2/include2_types.tcc"

namespace some::ns {

template void IncludedB::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t IncludedB::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t IncludedB::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t IncludedB::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;

} // namespace some::ns
