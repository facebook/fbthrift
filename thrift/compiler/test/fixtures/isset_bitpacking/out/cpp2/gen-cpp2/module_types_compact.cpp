/**
 * Autogenerated by Thrift for thrift/compiler/test/fixtures/isset_bitpacking/src/module.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated @nocommit
 */
#include "thrift/compiler/test/fixtures/isset_bitpacking/gen-cpp2/module_types.tcc"

namespace cpp2 {

template void Default::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t Default::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t Default::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t Default::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;

template void NonAtomic::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t NonAtomic::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t NonAtomic::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t NonAtomic::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;

template void Atomic::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t Atomic::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t Atomic::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t Atomic::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;

template void AtomicFoo::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t AtomicFoo::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t AtomicFoo::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t AtomicFoo::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;

} // namespace cpp2
