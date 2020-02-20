/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */

package test.fixtures.includes.transitive;

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
@ThriftStruct(value="Foo", builder=Foo.Builder.class)
public final class Foo {
    @ThriftConstructor
    public Foo(
        @ThriftField(value=1, name="a", requiredness=Requiredness.NONE) final long a
    ) {
        this.a = a;
    }
    
    @ThriftConstructor
    protected Foo() {
      this.a = 0L;
    }
    
    public static class Builder {
        private long a;
        @ThriftField(value=1, name="a", requiredness=Requiredness.NONE)
        public Builder setA(long a) {
            this.a = a;
            return this;
        }
    
        public Builder() { }
        public Builder(Foo other) {
            this.a = other.a;
        }
    
        @ThriftConstructor
        public Foo build() {
            return new Foo (
                this.a
            );
        }
    }
    
    private static final TStruct STRUCT_DESC = new TStruct("Foo");
    private final long a;
    public static final int _A = 1;
    private static final TField A_FIELD_DESC = new TField("a", TType.I64, (short)1);

    
    @ThriftField(value=1, name="a", requiredness=Requiredness.NONE)
    public long getA() { return a; }
    
    @Override
    public String toString() {
        return toStringHelper(this)
            .add("a", a)
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
    
        Foo other = (Foo)o;
    
        return
            Objects.equals(a, other.a) &&
            true;
    }
    
    @Override
    public int hashCode() {
        return Arrays.deepHashCode(new Object[] {
            a
        });
    }
    
    public void write0(TProtocol oprot) throws TException {
      oprot.writeStructBegin(STRUCT_DESC);
      oprot.writeFieldBegin(A_FIELD_DESC);
      oprot.writeI64(this.a);
      oprot.writeFieldEnd();
      oprot.writeFieldStop();
      oprot.writeStructEnd();
    }
    
}
