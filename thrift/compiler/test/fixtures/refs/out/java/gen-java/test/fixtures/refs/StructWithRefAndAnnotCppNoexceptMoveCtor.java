/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */

package test.fixtures.refs;

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
@com.facebook.swift.codec.ThriftStruct(value="StructWithRefAndAnnotCppNoexceptMoveCtor", builder=StructWithRefAndAnnotCppNoexceptMoveCtor.Builder.class)
public final class StructWithRefAndAnnotCppNoexceptMoveCtor implements com.facebook.thrift.payload.ThriftSerializable {
    @ThriftConstructor
    public StructWithRefAndAnnotCppNoexceptMoveCtor(
        @com.facebook.swift.codec.ThriftField(value=1, name="def_field", requiredness=Requiredness.NONE) final test.fixtures.refs.Empty defField
    ) {
        this.defField = defField;
    }
    
    @ThriftConstructor
    protected StructWithRefAndAnnotCppNoexceptMoveCtor() {
      this.defField = null;
    }

    public static Builder builder() {
      return new Builder();
    }

    public static Builder builder(StructWithRefAndAnnotCppNoexceptMoveCtor other) {
      return new Builder(other);
    }

    public static class Builder {
        private test.fixtures.refs.Empty defField = null;
    
        @com.facebook.swift.codec.ThriftField(value=1, name="def_field", requiredness=Requiredness.NONE)    public Builder setDefField(test.fixtures.refs.Empty defField) {
            this.defField = defField;
            return this;
        }
    
        public test.fixtures.refs.Empty getDefField() { return defField; }
    
        public Builder() { }
        public Builder(StructWithRefAndAnnotCppNoexceptMoveCtor other) {
            this.defField = other.defField;
        }
    
        @ThriftConstructor
        public StructWithRefAndAnnotCppNoexceptMoveCtor build() {
            StructWithRefAndAnnotCppNoexceptMoveCtor result = new StructWithRefAndAnnotCppNoexceptMoveCtor (
                this.defField
            );
            return result;
        }
    }
    
    public static final Map<String, Integer> NAMES_TO_IDS = new HashMap<>();
    public static final Map<String, Integer> THRIFT_NAMES_TO_IDS = new HashMap<>();
    public static final Map<Integer, TField> FIELD_METADATA = new HashMap<>();
    private static final TStruct STRUCT_DESC = new TStruct("StructWithRefAndAnnotCppNoexceptMoveCtor");
    private final test.fixtures.refs.Empty defField;
    public static final int _DEF_FIELD = 1;
    private static final TField DEF_FIELD_FIELD_DESC = new TField("def_field", TType.STRUCT, (short)1);
    static {
      NAMES_TO_IDS.put("defField", 1);
      THRIFT_NAMES_TO_IDS.put("def_field", 1);
      FIELD_METADATA.put(1, DEF_FIELD_FIELD_DESC);
    }
    
    @Nullable
    @com.facebook.swift.codec.ThriftField(value=1, name="def_field", requiredness=Requiredness.NONE)
    public test.fixtures.refs.Empty getDefField() { return defField; }

    @java.lang.Override
    public String toString() {
        ToStringHelper helper = toStringHelper(this);
        helper.add("defField", defField);
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
    
        StructWithRefAndAnnotCppNoexceptMoveCtor other = (StructWithRefAndAnnotCppNoexceptMoveCtor)o;
    
        return
            Objects.equals(defField, other.defField) &&
            true;
    }

    @java.lang.Override
    public int hashCode() {
        return Arrays.deepHashCode(new java.lang.Object[] {
            defField
        });
    }

    
    public static com.facebook.thrift.payload.Reader<StructWithRefAndAnnotCppNoexceptMoveCtor> asReader() {
      return StructWithRefAndAnnotCppNoexceptMoveCtor::read0;
    }
    
    public static StructWithRefAndAnnotCppNoexceptMoveCtor read0(TProtocol oprot) throws TException {
      TField __field;
      oprot.readStructBegin(StructWithRefAndAnnotCppNoexceptMoveCtor.NAMES_TO_IDS, StructWithRefAndAnnotCppNoexceptMoveCtor.THRIFT_NAMES_TO_IDS, StructWithRefAndAnnotCppNoexceptMoveCtor.FIELD_METADATA);
      StructWithRefAndAnnotCppNoexceptMoveCtor.Builder builder = new StructWithRefAndAnnotCppNoexceptMoveCtor.Builder();
      while (true) {
        __field = oprot.readFieldBegin();
        if (__field.type == TType.STOP) { break; }
        switch (__field.id) {
        case _DEF_FIELD:
          if (__field.type == TType.STRUCT) {
            test.fixtures.refs.Empty defField = test.fixtures.refs.Empty.read0(oprot);
            builder.setDefField(defField);
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
      if (defField != null) {
        oprot.writeFieldBegin(DEF_FIELD_FIELD_DESC);
        this.defField.write0(oprot);
        oprot.writeFieldEnd();
      }
      oprot.writeFieldStop();
      oprot.writeStructEnd();
    }

    private static class _StructWithRefAndAnnotCppNoexceptMoveCtorLazy {
        private static final StructWithRefAndAnnotCppNoexceptMoveCtor _DEFAULT = new StructWithRefAndAnnotCppNoexceptMoveCtor.Builder().build();
    }
    
    public static StructWithRefAndAnnotCppNoexceptMoveCtor defaultInstance() {
        return  _StructWithRefAndAnnotCppNoexceptMoveCtorLazy._DEFAULT;
    }
}
