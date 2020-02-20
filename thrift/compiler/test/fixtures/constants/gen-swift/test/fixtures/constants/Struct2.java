/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */

package test.fixtures.constants;

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
@ThriftStruct(value="struct2", builder=Struct2.Builder.class)
public final class Struct2 {
    @ThriftConstructor
    public Struct2(
        @ThriftField(value=1, name="a", requiredness=Requiredness.NONE) final int a,
        @ThriftField(value=2, name="b", requiredness=Requiredness.NONE) final String b,
        @ThriftField(value=3, name="c", requiredness=Requiredness.NONE) final test.fixtures.constants.Struct1 c,
        @ThriftField(value=4, name="d", requiredness=Requiredness.NONE) final List<Integer> d
    ) {
        this.a = a;
        this.b = b;
        this.c = c;
        this.d = d;
    }
    
    @ThriftConstructor
    protected Struct2() {
      this.a = 0;
      this.b = null;
      this.c = null;
      this.d = null;
    }
    
    public static class Builder {
        private int a;
        @ThriftField(value=1, name="a", requiredness=Requiredness.NONE)
        public Builder setA(int a) {
            this.a = a;
            return this;
        }
        private String b;
        @ThriftField(value=2, name="b", requiredness=Requiredness.NONE)
        public Builder setB(String b) {
            this.b = b;
            return this;
        }
        private test.fixtures.constants.Struct1 c;
        @ThriftField(value=3, name="c", requiredness=Requiredness.NONE)
        public Builder setC(test.fixtures.constants.Struct1 c) {
            this.c = c;
            return this;
        }
        private List<Integer> d;
        @ThriftField(value=4, name="d", requiredness=Requiredness.NONE)
        public Builder setD(List<Integer> d) {
            this.d = d;
            return this;
        }
    
        public Builder() { }
        public Builder(Struct2 other) {
            this.a = other.a;
            this.b = other.b;
            this.c = other.c;
            this.d = other.d;
        }
    
        @ThriftConstructor
        public Struct2 build() {
            return new Struct2 (
                this.a,
                this.b,
                this.c,
                this.d
            );
        }
    }
    
    private static final TStruct STRUCT_DESC = new TStruct("struct2");
    private final int a;
    public static final int _A = 1;
    private static final TField A_FIELD_DESC = new TField("a", TType.I32, (short)1);
    private final String b;
    public static final int _B = 2;
    private static final TField B_FIELD_DESC = new TField("b", TType.STRING, (short)2);
    private final test.fixtures.constants.Struct1 c;
    public static final int _C = 3;
    private static final TField C_FIELD_DESC = new TField("c", TType.STRUCT, (short)3);
    private final List<Integer> d;
    public static final int _D = 4;
    private static final TField D_FIELD_DESC = new TField("d", TType.LIST, (short)4);

    
    @ThriftField(value=1, name="a", requiredness=Requiredness.NONE)
    public int getA() { return a; }
        
    @ThriftField(value=2, name="b", requiredness=Requiredness.NONE)
    public String getB() { return b; }
        
    @ThriftField(value=3, name="c", requiredness=Requiredness.NONE)
    public test.fixtures.constants.Struct1 getC() { return c; }
        
    @ThriftField(value=4, name="d", requiredness=Requiredness.NONE)
    public List<Integer> getD() { return d; }
    
    @Override
    public String toString() {
        return toStringHelper(this)
            .add("a", a)
            .add("b", b)
            .add("c", c)
            .add("d", d)
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
    
        Struct2 other = (Struct2)o;
    
        return
            Objects.equals(a, other.a) &&
            Objects.equals(b, other.b) &&
            Objects.equals(c, other.c) &&
            Objects.equals(d, other.d) &&
            true;
    }
    
    @Override
    public int hashCode() {
        return Arrays.deepHashCode(new Object[] {
            a,
            b,
            c,
            d
        });
    }
    
    public void write0(TProtocol oprot) throws TException {
      oprot.writeStructBegin(STRUCT_DESC);
      oprot.writeFieldBegin(A_FIELD_DESC);
      oprot.writeI32(this.a);
      oprot.writeFieldEnd();
      if (this.b != null) {
        oprot.writeFieldBegin(B_FIELD_DESC);
        oprot.writeString(this.b);
        oprot.writeFieldEnd();
      }
      if (this.c != null) {
        oprot.writeFieldBegin(C_FIELD_DESC);
        this.c.write0(oprot);
        oprot.writeFieldEnd();
      }
      if (this.d != null) {
        oprot.writeFieldBegin(D_FIELD_DESC);
        List<Integer> _iter0 = this.d;
        oprot.writeListBegin(new TList(TType.I32, _iter0.size()));
        for (int _iter1 : _iter0) {
          oprot.writeI32(_iter1);
        }
        oprot.writeListEnd();
        oprot.writeFieldEnd();
      }
      oprot.writeFieldStop();
      oprot.writeStructEnd();
    }
    
}
