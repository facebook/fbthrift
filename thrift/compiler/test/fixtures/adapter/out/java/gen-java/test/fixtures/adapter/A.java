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
@com.facebook.swift.codec.ThriftStruct(value="A", builder=A.Builder.class)
public final class A implements com.facebook.thrift.payload.ThriftSerializable {
    @ThriftConstructor
    public A(
    ) {
    }
    
    
    
    public static Builder builder() {
      return new Builder();
    }

    public static Builder builder(A other) {
      return new Builder(other);
    }

    public static class Builder {
    
    public Builder() { }
        public Builder(A other) {
        }
    
        @ThriftConstructor
        public A build() {
            A result = new A (
            );
            return result;
        }
    }
        
    public static final Map<String, Integer> NAMES_TO_IDS = new HashMap<>();
    public static final Map<String, Integer> THRIFT_NAMES_TO_IDS = new HashMap<>();
    public static final Map<Integer, TField> FIELD_METADATA = new HashMap<>();
    private static final TStruct STRUCT_DESC = new TStruct("A");
static {
      com.facebook.thrift.type.TypeRegistry.add(new com.facebook.thrift.type.Type(
        new com.facebook.thrift.type.UniversalName("facebook.com/thrift/test/A"),
        A.class, A::read0));
    }
    @java.lang.Override
    public String toString() {
        ToStringHelper helper = toStringHelper(this);
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
    
        A other = (A)o;
    
        return
            true;
    }
    
    @java.lang.Override
    public int hashCode() {
        return Arrays.deepHashCode(new java.lang.Object[] {
        });
    }
    
    
    public static com.facebook.thrift.payload.Reader<A> asReader() {
      return A::read0;
    }
    
    public static A read0(TProtocol oprot) throws TException {
      TField __field;
      oprot.readStructBegin(A.NAMES_TO_IDS, A.THRIFT_NAMES_TO_IDS, A.FIELD_METADATA);
      A.Builder builder = new A.Builder();
      while (true) {
        __field = oprot.readFieldBegin();
        if (__field.type == TType.STOP) { break; }
        switch (__field.id) {
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
      oprot.writeFieldStop();
      oprot.writeStructEnd();
    }
    
    private static class _ALazy {
        private static final A _DEFAULT = new A.Builder().build();
    }
    
    public static A defaultInstance() {
        return  _ALazy._DEFAULT;
    }
}
