package test.fixtures.includes;

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
        @ThriftField(value=1, name="MyIncludedField", requiredness=Requiredness.NONE) final test.fixtures.includes.includes.Included myIncludedField
    ) {
        this.myIncludedField = myIncludedField;
    }

    public static class Builder {
        private test.fixtures.includes.includes.Included myIncludedField;

        public Builder setMyIncludedField(test.fixtures.includes.includes.Included myIncludedField) {
            this.myIncludedField = myIncludedField;
            return this;
        }

        public Builder() { }
        public Builder(MyStruct other) {
            this.myIncludedField = other.myIncludedField;
        }

        public MyStruct build() {
            return new MyStruct (
                this.myIncludedField
            );
        }
    }

    private final test.fixtures.includes.includes.Included myIncludedField;

    @ThriftField(value=1, name="MyIncludedField", requiredness=Requiredness.NONE)
    public test.fixtures.includes.includes.Included getMyIncludedField() { return myIncludedField; }

    @Override
    public String toString()
    {
        return toStringHelper(this)
            .add("myIncludedField", myIncludedField)
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
            Objects.equals(myIncludedField, other.myIncludedField);
    }

    @Override
    public int hashCode() {
        return Arrays.deepHashCode(new Object[] {
            myIncludedField
        });
    }
}
