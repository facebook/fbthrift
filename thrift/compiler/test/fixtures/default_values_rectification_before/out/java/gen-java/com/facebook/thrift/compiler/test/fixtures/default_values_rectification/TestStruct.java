/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */

package com.facebook.thrift.compiler.test.fixtures.default_values_rectification;

import com.facebook.swift.codec.*;
import com.facebook.swift.codec.ThriftField.Requiredness;
import com.facebook.swift.codec.ThriftField.Recursiveness;
import com.google.common.collect.*;
import java.util.*;
import javax.annotation.Nullable;
import org.apache.thrift.*;
import org.apache.thrift.transport.*;
import org.apache.thrift.protocol.*;
import static com.google.common.base.MoreObjects.toStringHelper;
import static com.google.common.base.MoreObjects.ToStringHelper;

@SwiftGenerated
@com.facebook.swift.codec.ThriftStruct(value="TestStruct", builder=TestStruct.Builder.class)
public final class TestStruct implements com.facebook.thrift.payload.ThriftSerializable {
    @ThriftConstructor
    public TestStruct(
        @com.facebook.swift.codec.ThriftField(value=1, name="unqualified_int_field", requiredness=Requiredness.NONE) final int unqualifiedIntField,
        @com.facebook.swift.codec.ThriftField(value=2, name="unqualified_bool_field", requiredness=Requiredness.NONE) final boolean unqualifiedBoolField,
        @com.facebook.swift.codec.ThriftField(value=3, name="unqualified_list_field", requiredness=Requiredness.NONE) final List<Integer> unqualifiedListField,
        @com.facebook.swift.codec.ThriftField(value=4, name="unqualified_struct_field", requiredness=Requiredness.NONE) final com.facebook.thrift.compiler.test.fixtures.default_values_rectification.EmptyStruct unqualifiedStructField,
        @com.facebook.swift.codec.ThriftField(value=5, name="optional_int_field", requiredness=Requiredness.OPTIONAL) final Integer optionalIntField,
        @com.facebook.swift.codec.ThriftField(value=6, name="optional_bool_field", requiredness=Requiredness.OPTIONAL) final Boolean optionalBoolField,
        @com.facebook.swift.codec.ThriftField(value=7, name="optional_list_field", requiredness=Requiredness.OPTIONAL) final List<Integer> optionalListField,
        @com.facebook.swift.codec.ThriftField(value=8, name="optional_struct_field", requiredness=Requiredness.OPTIONAL) final com.facebook.thrift.compiler.test.fixtures.default_values_rectification.EmptyStruct optionalStructField
    ) {
        this.unqualifiedIntField = unqualifiedIntField;
        this.unqualifiedBoolField = unqualifiedBoolField;
        this.unqualifiedListField = unqualifiedListField;
        this.unqualifiedStructField = unqualifiedStructField;
        this.optionalIntField = optionalIntField;
        this.optionalBoolField = optionalBoolField;
        this.optionalListField = optionalListField;
        this.optionalStructField = optionalStructField;
    }
    
    @ThriftConstructor
    protected TestStruct() {
      this.unqualifiedIntField = 0;
      this.unqualifiedBoolField = false;
      this.unqualifiedListField = ImmutableList.<Integer>builder()
            .build();
      this.unqualifiedStructField = new com.facebook.thrift.compiler.test.fixtures.default_values_rectification.EmptyStruct.Builder().build();
      this.optionalIntField = null;
      this.optionalBoolField = null;
      this.optionalListField = null;
      this.optionalStructField = null;
    }

    public static Builder builder() {
      return new Builder();
    }

    public static Builder builder(TestStruct other) {
      return new Builder(other);
    }

    public static class Builder {
        private int unqualifiedIntField = 0;
        private boolean unqualifiedBoolField = false;
        private List<Integer> unqualifiedListField = ImmutableList.<Integer>builder()
            .build();
        private com.facebook.thrift.compiler.test.fixtures.default_values_rectification.EmptyStruct unqualifiedStructField = new com.facebook.thrift.compiler.test.fixtures.default_values_rectification.EmptyStruct.Builder().build();
        private Integer optionalIntField = null;
        private Boolean optionalBoolField = null;
        private List<Integer> optionalListField = null;
        private com.facebook.thrift.compiler.test.fixtures.default_values_rectification.EmptyStruct optionalStructField = null;
    
        @com.facebook.swift.codec.ThriftField(value=1, name="unqualified_int_field", requiredness=Requiredness.NONE)    public Builder setUnqualifiedIntField(int unqualifiedIntField) {
            this.unqualifiedIntField = unqualifiedIntField;
            return this;
        }
    
        public int getUnqualifiedIntField() { return unqualifiedIntField; }
    
            @com.facebook.swift.codec.ThriftField(value=2, name="unqualified_bool_field", requiredness=Requiredness.NONE)    public Builder setUnqualifiedBoolField(boolean unqualifiedBoolField) {
            this.unqualifiedBoolField = unqualifiedBoolField;
            return this;
        }
    
        public boolean isUnqualifiedBoolField() { return unqualifiedBoolField; }
    
            @com.facebook.swift.codec.ThriftField(value=3, name="unqualified_list_field", requiredness=Requiredness.NONE)    public Builder setUnqualifiedListField(List<Integer> unqualifiedListField) {
            this.unqualifiedListField = unqualifiedListField;
            return this;
        }
    
        public List<Integer> getUnqualifiedListField() { return unqualifiedListField; }
    
            @com.facebook.swift.codec.ThriftField(value=4, name="unqualified_struct_field", requiredness=Requiredness.NONE)    public Builder setUnqualifiedStructField(com.facebook.thrift.compiler.test.fixtures.default_values_rectification.EmptyStruct unqualifiedStructField) {
            this.unqualifiedStructField = unqualifiedStructField;
            return this;
        }
    
        public com.facebook.thrift.compiler.test.fixtures.default_values_rectification.EmptyStruct getUnqualifiedStructField() { return unqualifiedStructField; }
    
            @com.facebook.swift.codec.ThriftField(value=5, name="optional_int_field", requiredness=Requiredness.OPTIONAL)    public Builder setOptionalIntField(Integer optionalIntField) {
            this.optionalIntField = optionalIntField;
            return this;
        }
    
        public Integer getOptionalIntField() { return optionalIntField; }
    
            @com.facebook.swift.codec.ThriftField(value=6, name="optional_bool_field", requiredness=Requiredness.OPTIONAL)    public Builder setOptionalBoolField(Boolean optionalBoolField) {
            this.optionalBoolField = optionalBoolField;
            return this;
        }
    
        public Boolean isOptionalBoolField() { return optionalBoolField; }
    
            @com.facebook.swift.codec.ThriftField(value=7, name="optional_list_field", requiredness=Requiredness.OPTIONAL)    public Builder setOptionalListField(List<Integer> optionalListField) {
            this.optionalListField = optionalListField;
            return this;
        }
    
        public List<Integer> getOptionalListField() { return optionalListField; }
    
            @com.facebook.swift.codec.ThriftField(value=8, name="optional_struct_field", requiredness=Requiredness.OPTIONAL)    public Builder setOptionalStructField(com.facebook.thrift.compiler.test.fixtures.default_values_rectification.EmptyStruct optionalStructField) {
            this.optionalStructField = optionalStructField;
            return this;
        }
    
        public com.facebook.thrift.compiler.test.fixtures.default_values_rectification.EmptyStruct getOptionalStructField() { return optionalStructField; }
    
        public Builder() { }
        public Builder(TestStruct other) {
            this.unqualifiedIntField = other.unqualifiedIntField;
            this.unqualifiedBoolField = other.unqualifiedBoolField;
            this.unqualifiedListField = other.unqualifiedListField;
            this.unqualifiedStructField = other.unqualifiedStructField;
            this.optionalIntField = other.optionalIntField;
            this.optionalBoolField = other.optionalBoolField;
            this.optionalListField = other.optionalListField;
            this.optionalStructField = other.optionalStructField;
        }
    
        @ThriftConstructor
        public TestStruct build() {
            TestStruct result = new TestStruct (
                this.unqualifiedIntField,
                this.unqualifiedBoolField,
                this.unqualifiedListField,
                this.unqualifiedStructField,
                this.optionalIntField,
                this.optionalBoolField,
                this.optionalListField,
                this.optionalStructField
            );
            return result;
        }
    }
    
    public static final Map<String, Integer> NAMES_TO_IDS = new HashMap<>();
    public static final Map<String, Integer> THRIFT_NAMES_TO_IDS = new HashMap<>();
    public static final Map<Integer, TField> FIELD_METADATA = new HashMap<>();
    private static final TStruct STRUCT_DESC = new TStruct("TestStruct");
    private final int unqualifiedIntField;
    public static final int _UNQUALIFIED_INT_FIELD = 1;
    private static final TField UNQUALIFIED_INT_FIELD_FIELD_DESC = new TField("unqualified_int_field", TType.I32, (short)1);
        private final boolean unqualifiedBoolField;
    public static final int _UNQUALIFIED_BOOL_FIELD = 2;
    private static final TField UNQUALIFIED_BOOL_FIELD_FIELD_DESC = new TField("unqualified_bool_field", TType.BOOL, (short)2);
        private final List<Integer> unqualifiedListField;
    public static final int _UNQUALIFIED_LIST_FIELD = 3;
    private static final TField UNQUALIFIED_LIST_FIELD_FIELD_DESC = new TField("unqualified_list_field", TType.LIST, (short)3);
        private final com.facebook.thrift.compiler.test.fixtures.default_values_rectification.EmptyStruct unqualifiedStructField;
    public static final int _UNQUALIFIED_STRUCT_FIELD = 4;
    private static final TField UNQUALIFIED_STRUCT_FIELD_FIELD_DESC = new TField("unqualified_struct_field", TType.STRUCT, (short)4);
        private final Integer optionalIntField;
    public static final int _OPTIONAL_INT_FIELD = 5;
    private static final TField OPTIONAL_INT_FIELD_FIELD_DESC = new TField("optional_int_field", TType.I32, (short)5);
        private final Boolean optionalBoolField;
    public static final int _OPTIONAL_BOOL_FIELD = 6;
    private static final TField OPTIONAL_BOOL_FIELD_FIELD_DESC = new TField("optional_bool_field", TType.BOOL, (short)6);
        private final List<Integer> optionalListField;
    public static final int _OPTIONAL_LIST_FIELD = 7;
    private static final TField OPTIONAL_LIST_FIELD_FIELD_DESC = new TField("optional_list_field", TType.LIST, (short)7);
        private final com.facebook.thrift.compiler.test.fixtures.default_values_rectification.EmptyStruct optionalStructField;
    public static final int _OPTIONAL_STRUCT_FIELD = 8;
    private static final TField OPTIONAL_STRUCT_FIELD_FIELD_DESC = new TField("optional_struct_field", TType.STRUCT, (short)8);
    static {
      NAMES_TO_IDS.put("unqualifiedIntField", 1);
      THRIFT_NAMES_TO_IDS.put("unqualified_int_field", 1);
      FIELD_METADATA.put(1, UNQUALIFIED_INT_FIELD_FIELD_DESC);
      NAMES_TO_IDS.put("unqualifiedBoolField", 2);
      THRIFT_NAMES_TO_IDS.put("unqualified_bool_field", 2);
      FIELD_METADATA.put(2, UNQUALIFIED_BOOL_FIELD_FIELD_DESC);
      NAMES_TO_IDS.put("unqualifiedListField", 3);
      THRIFT_NAMES_TO_IDS.put("unqualified_list_field", 3);
      FIELD_METADATA.put(3, UNQUALIFIED_LIST_FIELD_FIELD_DESC);
      NAMES_TO_IDS.put("unqualifiedStructField", 4);
      THRIFT_NAMES_TO_IDS.put("unqualified_struct_field", 4);
      FIELD_METADATA.put(4, UNQUALIFIED_STRUCT_FIELD_FIELD_DESC);
      NAMES_TO_IDS.put("optionalIntField", 5);
      THRIFT_NAMES_TO_IDS.put("optional_int_field", 5);
      FIELD_METADATA.put(5, OPTIONAL_INT_FIELD_FIELD_DESC);
      NAMES_TO_IDS.put("optionalBoolField", 6);
      THRIFT_NAMES_TO_IDS.put("optional_bool_field", 6);
      FIELD_METADATA.put(6, OPTIONAL_BOOL_FIELD_FIELD_DESC);
      NAMES_TO_IDS.put("optionalListField", 7);
      THRIFT_NAMES_TO_IDS.put("optional_list_field", 7);
      FIELD_METADATA.put(7, OPTIONAL_LIST_FIELD_FIELD_DESC);
      NAMES_TO_IDS.put("optionalStructField", 8);
      THRIFT_NAMES_TO_IDS.put("optional_struct_field", 8);
      FIELD_METADATA.put(8, OPTIONAL_STRUCT_FIELD_FIELD_DESC);
      com.facebook.thrift.type.TypeRegistry.add(new com.facebook.thrift.type.Type(
        new com.facebook.thrift.type.UniversalName("facebook.com/thrift/compiler/test/fixtures/default_values_rectification/TestStruct"),
        TestStruct.class, TestStruct::read0));
    }
    
    
    @com.facebook.swift.codec.ThriftField(value=1, name="unqualified_int_field", requiredness=Requiredness.NONE)
    public int getUnqualifiedIntField() { return unqualifiedIntField; }

    
    
    @com.facebook.swift.codec.ThriftField(value=2, name="unqualified_bool_field", requiredness=Requiredness.NONE)
    public boolean isUnqualifiedBoolField() { return unqualifiedBoolField; }

    
    @Nullable
    @com.facebook.swift.codec.ThriftField(value=3, name="unqualified_list_field", requiredness=Requiredness.NONE)
    public List<Integer> getUnqualifiedListField() { return unqualifiedListField; }

    
    @Nullable
    @com.facebook.swift.codec.ThriftField(value=4, name="unqualified_struct_field", requiredness=Requiredness.NONE)
    public com.facebook.thrift.compiler.test.fixtures.default_values_rectification.EmptyStruct getUnqualifiedStructField() { return unqualifiedStructField; }

    
    @Nullable
    @com.facebook.swift.codec.ThriftField(value=5, name="optional_int_field", requiredness=Requiredness.OPTIONAL)
    public Integer getOptionalIntField() { return optionalIntField; }

    
    @Nullable
    @com.facebook.swift.codec.ThriftField(value=6, name="optional_bool_field", requiredness=Requiredness.OPTIONAL)
    public Boolean isOptionalBoolField() { return optionalBoolField; }

    
    @Nullable
    @com.facebook.swift.codec.ThriftField(value=7, name="optional_list_field", requiredness=Requiredness.OPTIONAL)
    public List<Integer> getOptionalListField() { return optionalListField; }

    
    @Nullable
    @com.facebook.swift.codec.ThriftField(value=8, name="optional_struct_field", requiredness=Requiredness.OPTIONAL)
    public com.facebook.thrift.compiler.test.fixtures.default_values_rectification.EmptyStruct getOptionalStructField() { return optionalStructField; }

    @java.lang.Override
    public String toString() {
        ToStringHelper helper = toStringHelper(this);
        helper.add("unqualifiedIntField", unqualifiedIntField);
        helper.add("unqualifiedBoolField", unqualifiedBoolField);
        helper.add("unqualifiedListField", unqualifiedListField);
        helper.add("unqualifiedStructField", unqualifiedStructField);
        helper.add("optionalIntField", optionalIntField);
        helper.add("optionalBoolField", optionalBoolField);
        helper.add("optionalListField", optionalListField);
        helper.add("optionalStructField", optionalStructField);
        return helper.toString();
    }

    @java.lang.Override
    public boolean equals(java.lang.Object o) {
        if (this == o) {
            return true;
        }
        if (o == null || getClass() != o.getClass()) {
            return false;
        }
    
        TestStruct other = (TestStruct)o;
    
        return
            Objects.equals(unqualifiedIntField, other.unqualifiedIntField) &&
            Objects.equals(unqualifiedBoolField, other.unqualifiedBoolField) &&
            Objects.equals(unqualifiedListField, other.unqualifiedListField) &&
            Objects.equals(unqualifiedStructField, other.unqualifiedStructField) &&
            Objects.equals(optionalIntField, other.optionalIntField) &&
            Objects.equals(optionalBoolField, other.optionalBoolField) &&
            Objects.equals(optionalListField, other.optionalListField) &&
            Objects.equals(optionalStructField, other.optionalStructField) &&
            true;
    }

    @java.lang.Override
    public int hashCode() {
        return Arrays.deepHashCode(new java.lang.Object[] {
            unqualifiedIntField,
            unqualifiedBoolField,
            unqualifiedListField,
            unqualifiedStructField,
            optionalIntField,
            optionalBoolField,
            optionalListField,
            optionalStructField
        });
    }

    
    public static com.facebook.thrift.payload.Reader<TestStruct> asReader() {
      return TestStruct::read0;
    }
    
    public static TestStruct read0(TProtocol oprot) throws TException {
      TField __field;
      oprot.readStructBegin(TestStruct.NAMES_TO_IDS, TestStruct.THRIFT_NAMES_TO_IDS, TestStruct.FIELD_METADATA);
      TestStruct.Builder builder = new TestStruct.Builder();
      while (true) {
        __field = oprot.readFieldBegin();
        if (__field.type == TType.STOP) { break; }
        switch (__field.id) {
        case _UNQUALIFIED_INT_FIELD:
          if (__field.type == TType.I32) {
            int unqualifiedIntField = oprot.readI32();
            builder.setUnqualifiedIntField(unqualifiedIntField);
          } else {
            TProtocolUtil.skip(oprot, __field.type);
          }
          break;
        case _UNQUALIFIED_BOOL_FIELD:
          if (__field.type == TType.BOOL) {
            boolean unqualifiedBoolField = oprot.readBool();
            builder.setUnqualifiedBoolField(unqualifiedBoolField);
          } else {
            TProtocolUtil.skip(oprot, __field.type);
          }
          break;
        case _UNQUALIFIED_LIST_FIELD:
          if (__field.type == TType.LIST) {
            List<Integer> unqualifiedListField;
                {
                TList _list = oprot.readListBegin();
                unqualifiedListField = new ArrayList<Integer>(Math.max(0, _list.size));
                for (int _i = 0; (_list.size < 0) ? oprot.peekList() : (_i < _list.size); _i++) {
                    
                    int _value1 = oprot.readI32();
                    unqualifiedListField.add(_value1);
                }
                oprot.readListEnd();
                }
            builder.setUnqualifiedListField(unqualifiedListField);
          } else {
            TProtocolUtil.skip(oprot, __field.type);
          }
          break;
        case _UNQUALIFIED_STRUCT_FIELD:
          if (__field.type == TType.STRUCT) {
            com.facebook.thrift.compiler.test.fixtures.default_values_rectification.EmptyStruct unqualifiedStructField = com.facebook.thrift.compiler.test.fixtures.default_values_rectification.EmptyStruct.read0(oprot);
            builder.setUnqualifiedStructField(unqualifiedStructField);
          } else {
            TProtocolUtil.skip(oprot, __field.type);
          }
          break;
        case _OPTIONAL_INT_FIELD:
          if (__field.type == TType.I32) {
            Integer  optionalIntField = oprot.readI32();
            builder.setOptionalIntField(optionalIntField);
          } else {
            TProtocolUtil.skip(oprot, __field.type);
          }
          break;
        case _OPTIONAL_BOOL_FIELD:
          if (__field.type == TType.BOOL) {
            Boolean  optionalBoolField = oprot.readBool();
            builder.setOptionalBoolField(optionalBoolField);
          } else {
            TProtocolUtil.skip(oprot, __field.type);
          }
          break;
        case _OPTIONAL_LIST_FIELD:
          if (__field.type == TType.LIST) {
            List<Integer> optionalListField;
                {
                TList _list = oprot.readListBegin();
                optionalListField = new ArrayList<Integer>(Math.max(0, _list.size));
                for (int _i = 0; (_list.size < 0) ? oprot.peekList() : (_i < _list.size); _i++) {
                    
                    int _value1 = oprot.readI32();
                    optionalListField.add(_value1);
                }
                oprot.readListEnd();
                }
            builder.setOptionalListField(optionalListField);
          } else {
            TProtocolUtil.skip(oprot, __field.type);
          }
          break;
        case _OPTIONAL_STRUCT_FIELD:
          if (__field.type == TType.STRUCT) {
            com.facebook.thrift.compiler.test.fixtures.default_values_rectification.EmptyStruct optionalStructField = com.facebook.thrift.compiler.test.fixtures.default_values_rectification.EmptyStruct.read0(oprot);
            builder.setOptionalStructField(optionalStructField);
          } else {
            TProtocolUtil.skip(oprot, __field.type);
          }
          break;
        default:
          TProtocolUtil.skip(oprot, __field.type);
          break;
        }
        oprot.readFieldEnd();
      }
      oprot.readStructEnd();
      return builder.build();
    }

    public void write0(TProtocol oprot) throws TException {
      oprot.writeStructBegin(STRUCT_DESC);
      oprot.writeFieldBegin(UNQUALIFIED_INT_FIELD_FIELD_DESC);
      oprot.writeI32(this.unqualifiedIntField);
      oprot.writeFieldEnd();
      oprot.writeFieldBegin(UNQUALIFIED_BOOL_FIELD_FIELD_DESC);
      oprot.writeBool(this.unqualifiedBoolField);
      oprot.writeFieldEnd();
      if (unqualifiedListField != null) {
        oprot.writeFieldBegin(UNQUALIFIED_LIST_FIELD_FIELD_DESC);
        List<Integer> _iter0 = unqualifiedListField;
        oprot.writeListBegin(new TList(TType.I32, _iter0.size()));
            for (int _iter1 : _iter0) {
              oprot.writeI32(_iter1);
            }
            oprot.writeListEnd();
        oprot.writeFieldEnd();
      }
      if (unqualifiedStructField != null) {
        oprot.writeFieldBegin(UNQUALIFIED_STRUCT_FIELD_FIELD_DESC);
        this.unqualifiedStructField.write0(oprot);
        oprot.writeFieldEnd();
      }
      if (optionalIntField != null) {
        oprot.writeFieldBegin(OPTIONAL_INT_FIELD_FIELD_DESC);
        oprot.writeI32(this.optionalIntField);
        oprot.writeFieldEnd();
      }
      if (optionalBoolField != null) {
        oprot.writeFieldBegin(OPTIONAL_BOOL_FIELD_FIELD_DESC);
        oprot.writeBool(this.optionalBoolField);
        oprot.writeFieldEnd();
      }
      if (optionalListField != null) {
        oprot.writeFieldBegin(OPTIONAL_LIST_FIELD_FIELD_DESC);
        List<Integer>  _iter0 = optionalListField;
        oprot.writeListBegin(new TList(TType.I32, _iter0.size()));
            for (int _iter1 : _iter0) {
              oprot.writeI32(_iter1);
            }
            oprot.writeListEnd();
        oprot.writeFieldEnd();
      }
      if (optionalStructField != null) {
        oprot.writeFieldBegin(OPTIONAL_STRUCT_FIELD_FIELD_DESC);
        this.optionalStructField.write0(oprot);
        oprot.writeFieldEnd();
      }
      oprot.writeFieldStop();
      oprot.writeStructEnd();
    }

    private static class _TestStructLazy {
        private static final TestStruct _DEFAULT = new TestStruct.Builder().build();
    }
    
    public static TestStruct defaultInstance() {
        return  _TestStructLazy._DEFAULT;
    }
}
