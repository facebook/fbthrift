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
@com.facebook.swift.codec.ThriftStruct(value="DirectlyAdaptedStruct", builder=DirectlyAdaptedStruct.Builder.class)
public final class DirectlyAdaptedStruct implements com.facebook.thrift.payload.ThriftSerializable {
    @ThriftConstructor
    public DirectlyAdaptedStruct(
        @com.facebook.swift.codec.ThriftField(value=1, name="data", requiredness=Requiredness.NONE) final long data
    ) {
        this.data = data;
    }
    
    @ThriftConstructor
    protected DirectlyAdaptedStruct() {
      this.data = 0L;
    }

    public static Builder builder() {
      return new Builder();
    }

    public static Builder builder(DirectlyAdaptedStruct other) {
      return new Builder(other);
    }

    public static class Builder {
        private long data = 0L;
    
        @com.facebook.swift.codec.ThriftField(value=1, name="data", requiredness=Requiredness.NONE)    public Builder setData(long data) {
            this.data = data;
            return this;
        }
    
        public long getData() { return data; }
    
        public Builder() { }
        public Builder(DirectlyAdaptedStruct other) {
            this.data = other.data;
        }
    
        @ThriftConstructor
        public DirectlyAdaptedStruct build() {
            DirectlyAdaptedStruct result = new DirectlyAdaptedStruct (
                this.data
            );
            return result;
        }
    }
    
    public static final Map<String, Integer> NAMES_TO_IDS = new HashMap<>();
    public static final Map<String, Integer> THRIFT_NAMES_TO_IDS = new HashMap<>();
    public static final Map<Integer, TField> FIELD_METADATA = new HashMap<>();
    private static final TStruct STRUCT_DESC = new TStruct("DirectlyAdaptedStruct");
    private final long data;
    public static final int _DATA = 1;
    private static final TField DATA_FIELD_DESC = new TField("data", TType.I64, (short)1);
    static {
      NAMES_TO_IDS.put("data", 1);
      THRIFT_NAMES_TO_IDS.put("data", 1);
      FIELD_METADATA.put(1, DATA_FIELD_DESC);
      com.facebook.thrift.type.TypeRegistry.add(new com.facebook.thrift.type.Type(
        new com.facebook.thrift.type.UniversalName("facebook.com/thrift/test/DirectlyAdaptedStruct"),
        DirectlyAdaptedStruct.class, DirectlyAdaptedStruct::read0));
    }
    
    
    @com.facebook.swift.codec.ThriftField(value=1, name="data", requiredness=Requiredness.NONE)
    public long getData() { return data; }

    @java.lang.Override
    public String toString() {
        ToStringHelper helper = toStringHelper(this);
        helper.add("data", data);
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
    
        DirectlyAdaptedStruct other = (DirectlyAdaptedStruct)o;
    
        return
            Objects.equals(data, other.data) &&
            true;
    }

    @java.lang.Override
    public int hashCode() {
        return Arrays.deepHashCode(new java.lang.Object[] {
            data
        });
    }

    
    public static com.facebook.thrift.payload.Reader<DirectlyAdaptedStruct> asReader() {
      return DirectlyAdaptedStruct::read0;
    }
    
    public static DirectlyAdaptedStruct read0(TProtocol oprot) throws TException {
      TField __field;
      oprot.readStructBegin(DirectlyAdaptedStruct.NAMES_TO_IDS, DirectlyAdaptedStruct.THRIFT_NAMES_TO_IDS, DirectlyAdaptedStruct.FIELD_METADATA);
      DirectlyAdaptedStruct.Builder builder = new DirectlyAdaptedStruct.Builder();
      while (true) {
        __field = oprot.readFieldBegin();
        if (__field.type == TType.STOP) { break; }
        switch (__field.id) {
        case _DATA:
          if (__field.type == TType.I64) {
            long data = oprot.readI64();
            builder.setData(data);
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
      oprot.writeFieldBegin(DATA_FIELD_DESC);
      oprot.writeI64(this.data);
      oprot.writeFieldEnd();
      oprot.writeFieldStop();
      oprot.writeStructEnd();
    }

    private static class _DirectlyAdaptedStructLazy {
        private static final DirectlyAdaptedStruct _DEFAULT = new DirectlyAdaptedStruct.Builder().build();
    }
    
    public static DirectlyAdaptedStruct defaultInstance() {
        return  _DirectlyAdaptedStructLazy._DEFAULT;
    }
}
