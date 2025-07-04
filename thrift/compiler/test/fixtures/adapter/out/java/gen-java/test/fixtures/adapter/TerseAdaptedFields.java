/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */

package test.fixtures.adapter;

import com.facebook.swift.codec.*;
import com.facebook.swift.codec.ThriftField.Requiredness;
import com.facebook.swift.codec.ThriftField.Recursiveness;
import com.google.common.collect.*;
import java.util.*;
import javax.annotation.Nullable;
import org.apache.thrift.*;
import org.apache.thrift.TException;
import org.apache.thrift.transport.*;
import org.apache.thrift.protocol.*;
import org.apache.thrift.protocol.TProtocol;
import static com.google.common.base.MoreObjects.toStringHelper;
import static com.google.common.base.MoreObjects.ToStringHelper;

@SwiftGenerated
@com.facebook.swift.codec.ThriftStruct(value="TerseAdaptedFields", builder=TerseAdaptedFields.Builder.class)
public final class TerseAdaptedFields implements com.facebook.thrift.payload.ThriftSerializable {
    @ThriftConstructor
    public TerseAdaptedFields(
        @com.facebook.swift.codec.ThriftField(value=1, name="int_field", requiredness=Requiredness.TERSE) final int intField,
        @com.facebook.swift.codec.ThriftField(value=2, name="string_field", requiredness=Requiredness.TERSE) final String stringField,
        @com.facebook.swift.codec.ThriftField(value=3, name="set_field", requiredness=Requiredness.TERSE) final Set<Integer> setField
    ) {
        this.intField = intField;
        this.stringField = stringField;
        this.setField = setField;
    }
    
    @ThriftConstructor
    protected TerseAdaptedFields() {
      this.intField = 0;
      this.stringField = com.facebook.thrift.util.IntrinsicDefaults.defaultString();
      this.setField = com.facebook.thrift.util.IntrinsicDefaults.defaultSet();
    }

    public static Builder builder() {
      return new Builder();
    }

    public static Builder builder(TerseAdaptedFields other) {
      return new Builder(other);
    }

    public static class Builder {
        private int intField = 0;
        private String stringField = com.facebook.thrift.util.IntrinsicDefaults.defaultString();
        private Set<Integer> setField = com.facebook.thrift.util.IntrinsicDefaults.defaultSet();
    
        @com.facebook.swift.codec.ThriftField(value=1, name="int_field", requiredness=Requiredness.TERSE)    public Builder setIntField(int intField) {
            this.intField = intField;
            return this;
        }
    
        public int getIntField() { return intField; }
    
            @com.facebook.swift.codec.ThriftField(value=2, name="string_field", requiredness=Requiredness.TERSE)    public Builder setStringField(String stringField) {
            this.stringField = stringField;
            return this;
        }
    
        public String getStringField() { return stringField; }
    
            @com.facebook.swift.codec.ThriftField(value=3, name="set_field", requiredness=Requiredness.TERSE)    public Builder setSetField(Set<Integer> setField) {
            this.setField = setField;
            return this;
        }
    
        public Set<Integer> getSetField() { return setField; }
    
        public Builder() { }
        public Builder(TerseAdaptedFields other) {
            this.intField = other.intField;
            this.stringField = other.stringField;
            this.setField = other.setField;
        }
    
        @ThriftConstructor
        public TerseAdaptedFields build() {
            TerseAdaptedFields result = new TerseAdaptedFields (
                this.intField,
                this.stringField,
                this.setField
            );
            return result;
        }
    }
    
    public static final Map<String, Integer> NAMES_TO_IDS = new HashMap<>();
    public static final Map<String, Integer> THRIFT_NAMES_TO_IDS = new HashMap<>();
    public static final Map<Integer, TField> FIELD_METADATA = new HashMap<>();
    private static final TStruct STRUCT_DESC = new TStruct("TerseAdaptedFields");
    private final int intField;
    public static final int _INT_FIELD = 1;
    private static final TField INT_FIELD_FIELD_DESC = new TField("int_field", TType.I32, (short)1);
        private final String stringField;
    public static final int _STRING_FIELD = 2;
    private static final TField STRING_FIELD_FIELD_DESC = new TField("string_field", TType.STRING, (short)2);
        private final Set<Integer> setField;
    public static final int _SET_FIELD = 3;
    private static final TField SET_FIELD_FIELD_DESC = new TField("set_field", TType.SET, (short)3);
    static {
      NAMES_TO_IDS.put("intField", 1);
      THRIFT_NAMES_TO_IDS.put("int_field", 1);
      FIELD_METADATA.put(1, INT_FIELD_FIELD_DESC);
      NAMES_TO_IDS.put("stringField", 2);
      THRIFT_NAMES_TO_IDS.put("string_field", 2);
      FIELD_METADATA.put(2, STRING_FIELD_FIELD_DESC);
      NAMES_TO_IDS.put("setField", 3);
      THRIFT_NAMES_TO_IDS.put("set_field", 3);
      FIELD_METADATA.put(3, SET_FIELD_FIELD_DESC);
      com.facebook.thrift.type.TypeRegistry.add(new com.facebook.thrift.type.Type(
        new com.facebook.thrift.type.UniversalName("facebook.com/thrift/test/TerseAdaptedFields"),
        TerseAdaptedFields.class, TerseAdaptedFields::read0));
    }
    
    
    @com.facebook.swift.codec.ThriftField(value=1, name="int_field", requiredness=Requiredness.TERSE)
    public int getIntField() { return intField; }

    
    @Nullable
    @com.facebook.swift.codec.ThriftField(value=2, name="string_field", requiredness=Requiredness.TERSE)
    public String getStringField() { return stringField; }

    
    @Nullable
    @com.facebook.swift.codec.ThriftField(value=3, name="set_field", requiredness=Requiredness.TERSE)
    public Set<Integer> getSetField() { return setField; }

    @java.lang.Override
    public String toString() {
        ToStringHelper helper = toStringHelper(this);
        helper.add("intField", intField);
        helper.add("stringField", stringField);
        helper.add("setField", setField);
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
    
        TerseAdaptedFields other = (TerseAdaptedFields)o;
    
        return
            Objects.equals(intField, other.intField) &&
            Objects.equals(stringField, other.stringField) &&
            Objects.equals(setField, other.setField) &&
            true;
    }

    @java.lang.Override
    public int hashCode() {
        return Arrays.deepHashCode(new java.lang.Object[] {
            intField,
            stringField,
            setField
        });
    }

    
    public static com.facebook.thrift.payload.Reader<TerseAdaptedFields> asReader() {
      return TerseAdaptedFields::read0;
    }
    
    public static TerseAdaptedFields read0(TProtocol oprot) throws TException {
      TField __field;
      oprot.readStructBegin(TerseAdaptedFields.NAMES_TO_IDS, TerseAdaptedFields.THRIFT_NAMES_TO_IDS, TerseAdaptedFields.FIELD_METADATA);
      TerseAdaptedFields.Builder builder = new TerseAdaptedFields.Builder();
      while (true) {
        __field = oprot.readFieldBegin();
        if (__field.type == TType.STOP) { break; }
        switch (__field.id) {
        case _INT_FIELD:
          if (__field.type == TType.I32) {
            int intField = oprot.readI32();
            builder.setIntField(intField);
          } else {
            TProtocolUtil.skip(oprot, __field.type);
          }
          break;
        case _STRING_FIELD:
          if (__field.type == TType.STRING) {
            String stringField = oprot.readString();
            builder.setStringField(stringField);
          } else {
            TProtocolUtil.skip(oprot, __field.type);
          }
          break;
        case _SET_FIELD:
          if (__field.type == TType.SET) {
            Set<Integer> setField;
                {
                TSet _set = oprot.readSetBegin();
                setField = new HashSet<Integer>(Math.max(0, _set.size));
                for (int _i = 0; (_set.size < 0) ? oprot.peekSet() : (_i < _set.size); _i++) {
                    
                    int _value1 = oprot.readI32();
                    setField.add(_value1);
                }
                oprot.readSetEnd();
                }
            builder.setSetField(setField);
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
      int structStart = 0;
      int pos = 0;
      com.facebook.thrift.protocol.ByteBufTProtocol p = (com.facebook.thrift.protocol.ByteBufTProtocol) oprot;
      if (!com.facebook.thrift.util.IntrinsicDefaults.isDefault(intField)) {
        oprot.writeFieldBegin(INT_FIELD_FIELD_DESC);
        oprot.writeI32(this.intField);
        oprot.writeFieldEnd();
      };
      java.util.Objects.requireNonNull(stringField, "stringField must not be null");
      
      if (!com.facebook.thrift.util.IntrinsicDefaults.isDefault(stringField)) {
        oprot.writeFieldBegin(STRING_FIELD_FIELD_DESC);
        oprot.writeString(this.stringField);
        oprot.writeFieldEnd();
      }
      java.util.Objects.requireNonNull(setField, "setField must not be null");
      
      if (!com.facebook.thrift.util.IntrinsicDefaults.isDefault(setField)) {
        oprot.writeFieldBegin(SET_FIELD_FIELD_DESC);
        Set<Integer> _iter0 = setField;
        oprot.writeSetBegin(new TSet(TType.I32, _iter0.size()));
            for (int _iter1 : _iter0) {
              oprot.writeI32(_iter1);
            }
            oprot.writeSetEnd();
        oprot.writeFieldEnd();
      }
      oprot.writeFieldStop();
      oprot.writeStructEnd();
    }

    private static class _TerseAdaptedFieldsLazy {
        private static final TerseAdaptedFields _DEFAULT = new TerseAdaptedFields.Builder().build();
    }
    
    public static TerseAdaptedFields defaultInstance() {
        return  _TerseAdaptedFieldsLazy._DEFAULT;
    }
}
