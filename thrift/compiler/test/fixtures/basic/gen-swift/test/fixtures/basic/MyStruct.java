package test.fixtures.basic;

import com.facebook.swift.codec.*;
import com.facebook.swift.codec.ThriftField.Requiredness;
import com.facebook.swift.codec.ThriftField.Recursiveness;
import java.util.*;

import static com.google.common.base.MoreObjects.toStringHelper;

@ThriftStruct("MyStruct")
public final class MyStruct
{
    @ThriftConstructor
    public MyStruct(
        @ThriftField(value=1, name="MyIntField", requiredness=Requiredness.NONE) final long myIntField,
        @ThriftField(value=2, name="MyStringField", requiredness=Requiredness.NONE) final String myStringField
    ) {
        this.myIntField = myIntField;
        this.myStringField = myStringField;
    }

    public static class Builder {
        private long myIntField;

        public Builder setMyIntField(long myIntField) {
            this.myIntField = myIntField;
            return this;
        }
        private String myStringField;

        public Builder setMyStringField(String myStringField) {
            this.myStringField = myStringField;
            return this;
        }

        public Builder() { }
        public Builder(MyStruct other) {
            this.myIntField = other.myIntField;
            this.myStringField = other.myStringField;
        }

        public MyStruct build() {
            return new MyStruct (
                this.myIntField,
                this.myStringField
            );
        }
    }

    private final long myIntField;

    @ThriftField(value=1, name="MyIntField", requiredness=Requiredness.NONE)
    public long getMyIntField() { return myIntField; }

    private final String myStringField;

    @ThriftField(value=2, name="MyStringField", requiredness=Requiredness.NONE)
    public String getMyStringField() { return myStringField; }

    @Override
    public String toString()
    {
        return toStringHelper(this)
            .add("myIntField", myIntField)
            .add("myStringField", myStringField)
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

        MyStruct other = (MyStruct)o;

        return
            Objects.equals(myIntField, other.myIntField) &&
            Objects.equals(myStringField, other.myStringField);
    }

    @Override
    public int hashCode() {
        return Arrays.deepHashCode(new Object[] {
            myIntField,
            myStringField
        });
    }
}
