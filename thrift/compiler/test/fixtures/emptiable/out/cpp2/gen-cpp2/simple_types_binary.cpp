/**
 * Autogenerated by Thrift for thrift/compiler/test/fixtures/emptiable/src/simple.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated @nocommit
 */
#include "thrift/compiler/test/fixtures/emptiable/gen-cpp2/simple_types.tcc"

namespace apache::thrift::test {

template void MyStruct::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t MyStruct::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t MyStruct::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t MyStruct::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;

template void EmptiableStruct::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t EmptiableStruct::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t EmptiableStruct::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t EmptiableStruct::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;

template void EmptiableTerseStruct::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t EmptiableTerseStruct::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t EmptiableTerseStruct::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t EmptiableTerseStruct::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;

template void NotEmptiableStruct::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t NotEmptiableStruct::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t NotEmptiableStruct::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t NotEmptiableStruct::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;

} // namespace apache::thrift::test
