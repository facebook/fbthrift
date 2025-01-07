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
import org.apache.thrift.transport.*;
import org.apache.thrift.protocol.*;
import static com.google.common.base.MoreObjects.toStringHelper;
import static com.google.common.base.MoreObjects.ToStringHelper;

@SwiftGenerated
@com.facebook.swift.codec.ThriftStruct(value="CircularStruct", builder=CircularStruct.Builder.class)
public final class CircularStruct implements com.facebook.thrift.payload.ThriftSerializable {
    @ThriftConstructor
    public CircularStruct(
        @com.facebook.swift.codec.ThriftField(value=1, name="field", requiredness=Requiredness.OPTIONAL) final test.fixtures.adapter.CircularAdaptee field
    ) {
        this.field = field;
    }
    
    @ThriftConstructor
    protected CircularStruct() {
      this.field = null;
    }

    public static Builder builder() {
      return new Builder();
    }

    public static Builder builder(CircularStruct other) {
      return new Builder(other);
    }

    public static class Builder {
        private test.fixtures.adapter.CircularAdaptee field = null;
    
        @com.facebook.swift.codec.ThriftField(value=1, name="field", requiredness=Requiredness.OPTIONAL)    public Builder setField(test.fixtures.adapter.CircularAdaptee field) {
            this.field = field;
            return this;
        }
    
        public test.fixtures.adapter.CircularAdaptee getField() { return field; }
    
        public Builder() { }
        public Builder(CircularStruct other) {
            this.field = other.field;
        }
    
        @ThriftConstructor
        public CircularStruct build() {
            CircularStruct result = new CircularStruct (
                this.field
            );
            return result;
        }
    }
    
    public static final Map<String, Integer> NAMES_TO_IDS = new HashMap<>();
    public static final Map<String, Integer> THRIFT_NAMES_TO_IDS = new HashMap<>();
    public static final Map<Integer, TField> FIELD_METADATA = new HashMap<>();
    private static final TStruct STRUCT_DESC = new TStruct("CircularStruct");
    private final test.fixtures.adapter.CircularAdaptee field;
    public static final int _FIELD = 1;
    private static final TField FIELD_FIELD_DESC = new TField("field", TType.STRUCT, (short)1);
    static {
      NAMES_TO_IDS.put("field", 1);
      THRIFT_NAMES_TO_IDS.put("field", 1);
      FIELD_METADATA.put(1, FIELD_FIELD_DESC);
      com.facebook.thrift.type.TypeRegistry.add(new com.facebook.thrift.type.Type(
        new com.facebook.thrift.type.UniversalName("facebook.com/thrift/test/CircularStruct"),
        CircularStruct.class, CircularStruct::read0));
    }
    
    @Nullable
    @com.facebook.swift.codec.ThriftField(value=1, name="field", requiredness=Requiredness.OPTIONAL)
    public test.fixtures.adapter.CircularAdaptee getField() { return field; }

    @java.lang.Override
    public String toString() {
        ToStringHelper helper = toStringHelper(this);
        helper.add("field", field);
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
    
        CircularStruct other = (CircularStruct)o;
    
        return
            Objects.equals(field, other.field) &&
            true;
    }

    @java.lang.Override
    public int hashCode() {
        return Arrays.deepHashCode(new java.lang.Object[] {
            field
        });
    }

    
    public static com.facebook.thrift.payload.Reader<CircularStruct> asReader() {
      return CircularStruct::read0;
    }
    
    public static CircularStruct read0(TProtocol oprot) throws TException {
      TField __field;
      oprot.readStructBegin(CircularStruct.NAMES_TO_IDS, CircularStruct.THRIFT_NAMES_TO_IDS, CircularStruct.FIELD_METADATA);
      CircularStruct.Builder builder = new CircularStruct.Builder();
      while (true) {
        __field = oprot.readFieldBegin();
        if (__field.type == TType.STOP) { break; }
        switch (__field.id) {
        case _FIELD:
          if (__field.type == TType.STRUCT) {
            test.fixtures.adapter.CircularAdaptee field = test.fixtures.adapter.CircularAdaptee.read0(oprot);
            builder.setField(field);
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
      if (field != null) {
        oprot.writeFieldBegin(FIELD_FIELD_DESC);
        this.field.write0(oprot);
        oprot.writeFieldEnd();
      }
      oprot.writeFieldStop();
      oprot.writeStructEnd();
    }

    private static class _CircularStructLazy {
        private static final CircularStruct _DEFAULT = new CircularStruct.Builder().build();
    }
    
    public static CircularStruct defaultInstance() {
        return  _CircularStructLazy._DEFAULT;
    }
}
