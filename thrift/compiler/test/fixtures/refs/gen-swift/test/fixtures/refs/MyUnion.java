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
import java.util.*;
import org.apache.thrift.*;
import org.apache.thrift.async.*;
import org.apache.thrift.server.*;
import org.apache.thrift.transport.*;
import org.apache.thrift.protocol.*;

import static com.google.common.base.MoreObjects.toStringHelper;

@SwiftGenerated
@ThriftUnion("MyUnion")
public final class MyUnion {
    private static final Map<Short, String> ID_TO_THRIFT_NAME = new HashMap();
    static {
      ID_TO_THRIFT_NAME.put((short) 1, "anInteger");
      ID_TO_THRIFT_NAME.put((short) 2, "aString");
    }
    private Object value;
    private short id;
    
    private static final TStruct STRUCT_DESC = new TStruct("MyUnion");
    public static final int _ANINTEGER = 1;
    private static final TField AN_INTEGER_FIELD_DESC = new TField("anInteger", TType.I32, (short)1);
    public static final int _ASTRING = 2;
    private static final TField A_STRING_FIELD_DESC = new TField("aString", TType.STRING, (short)2);
    
    @ThriftConstructor
    public MyUnion() {
    }
    
    @ThriftConstructor
    @Deprecated
    public MyUnion(final int anInteger) {
        this.value = anInteger;
        this.id = 1;
    }
    
    @ThriftConstructor
    @Deprecated
    public MyUnion(final String aString) {
        this.value = aString;
        this.id = 2;
    }
    
    public static MyUnion fromAnInteger(final int anInteger) {
        MyUnion res = new MyUnion();
        res.value = anInteger;
        res.id = 1;
        return res;
    }
    
    public static MyUnion fromAString(final String aString) {
        MyUnion res = new MyUnion();
        res.value = aString;
        res.id = 2;
        return res;
    }
    

    @ThriftField(value=1, name="anInteger", requiredness=Requiredness.NONE)
    public int getAnInteger() {
        if (this.id != 1) {
            throw new IllegalStateException("Not a anInteger element!");
        }
        return (int) value;
    }

    public boolean isSetAnInteger() {
        return this.id == 1;
    }

    @ThriftField(value=2, name="aString", requiredness=Requiredness.NONE)
    public String getAString() {
        if (this.id != 2) {
            throw new IllegalStateException("Not a aString element!");
        }
        return (String) value;
    }

    public boolean isSetAString() {
        return this.id == 2;
    }

    @ThriftUnionId
    public short getThriftId() {
        return this.id;
    }

    public String getThriftName() {
        return ID_TO_THRIFT_NAME.get(this.id);
    }

    public void accept(Visitor visitor) {
        if (isSetAnInteger()) {
            visitor.visitAnInteger(getAnInteger());
            return;
        }
        if (isSetAString()) {
            visitor.visitAString(getAString());
            return;
        }
    }

    @Override
    public String toString() {
        return toStringHelper(this)
            .add("value", value)
            .add("id", id)
            .add("name", getThriftName())
            .add("type", value == null ? "<null>" : value.getClass().getSimpleName())
            .toString();
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) {
            return true;
        }
        if (o == null || getClass() != o.getClass()) {
            return false;
        }

        MyUnion other = (MyUnion)o;

        return Objects.equals(this.id, other.id)
                && Objects.deepEquals(this.value, other.value);
    }

    @Override
    public int hashCode() {
        return Arrays.deepHashCode(new Object[] {
            id,
            value,
        });
    }

    public interface Visitor {
        void visitAnInteger(int anInteger);
        void visitAString(String aString);
    }

    public void write0(TProtocol oprot) throws TException {
      oprot.writeStructBegin(STRUCT_DESC);
      if (this.id != 0 && this.value == null ){
         throw new TProtocolException("Cannot write a Union with marked-as-set but null value!");
      }
      switch (this.id) {
      case _ANINTEGER: {
        oprot.writeFieldBegin(AN_INTEGER_FIELD_DESC);
        int anInteger = (int)this.value;
        oprot.writeI32(anInteger);
        oprot.writeFieldEnd();
        break;
      }
      case _ASTRING: {
        oprot.writeFieldBegin(A_STRING_FIELD_DESC);
        String aString = (String)this.value;
        oprot.writeString(aString);
        oprot.writeFieldEnd();
        break;
      }
      default:
          throw new IllegalStateException("Cannot write union with unknown field ");
      }
      oprot.writeFieldStop();
      oprot.writeStructEnd();
    }
    
}
