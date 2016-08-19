package test.fixtures.req_opt;

import com.facebook.swift.codec.*;
import com.facebook.swift.codec.ThriftField.Requiredness;
import com.facebook.swift.codec.ThriftField.Recursiveness;
import java.util.*;

import static com.google.common.base.MoreObjects.toStringHelper;

@ThriftStruct("Foo")
public final class Foo
{
    @ThriftConstructor
    public Foo(
        @ThriftField(value=1, name="myInteger", requiredness=Requiredness.REQUIRED) final int myInteger,
        @ThriftField(value=2, name="myString", requiredness=Requiredness.OPTIONAL) final String myString,
        @ThriftField(value=3, name="myBools", requiredness=Requiredness.NONE) final List<Boolean> myBools,
        @ThriftField(value=4, name="myNumbers", requiredness=Requiredness.REQUIRED) final List<Integer> myNumbers
    ) {
        this.myInteger = myInteger;
        this.myString = myString;
        this.myBools = myBools;
        this.myNumbers = myNumbers;
    }

    public static class Builder {
        private int myInteger;

        public Builder setMyInteger(int myInteger) {
            this.myInteger = myInteger;
            return this;
        }
        private String myString;

        public Builder setMyString(String myString) {
            this.myString = myString;
            return this;
        }
        private List<Boolean> myBools;

        public Builder setMyBools(List<Boolean> myBools) {
            this.myBools = myBools;
            return this;
        }
        private List<Integer> myNumbers;

        public Builder setMyNumbers(List<Integer> myNumbers) {
            this.myNumbers = myNumbers;
            return this;
        }

        public Builder() { }
        public Builder(Foo other) {
            this.myInteger = other.myInteger;
            this.myString = other.myString;
            this.myBools = other.myBools;
            this.myNumbers = other.myNumbers;
        }

        public Foo build() {
            return new Foo (
                this.myInteger,
                this.myString,
                this.myBools,
                this.myNumbers
            );
        }
    }

    private final int myInteger;

    @ThriftField(value=1, name="myInteger", requiredness=Requiredness.REQUIRED)
    public int getMyInteger() { return myInteger; }

    private final String myString;

    @ThriftField(value=2, name="myString", requiredness=Requiredness.OPTIONAL)
    public String getMyString() { return myString; }

    private final List<Boolean> myBools;

    @ThriftField(value=3, name="myBools", requiredness=Requiredness.NONE)
    public List<Boolean> getMyBools() { return myBools; }

    private final List<Integer> myNumbers;

    @ThriftField(value=4, name="myNumbers", requiredness=Requiredness.REQUIRED)
    public List<Integer> getMyNumbers() { return myNumbers; }

    @Override
    public String toString()
    {
        return toStringHelper(this)
            .add("myInteger", myInteger)
            .add("myString", myString)
            .add("myBools", myBools)
            .add("myNumbers", myNumbers)
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
            Objects.equals(myInteger, other.myInteger) &&
            Objects.equals(myString, other.myString) &&
            Objects.equals(myBools, other.myBools) &&
            Objects.equals(myNumbers, other.myNumbers);
    }

    @Override
    public int hashCode() {
        return Arrays.deepHashCode(new Object[] {
            myInteger,
            myString,
            myBools,
            myNumbers
        });
    }
}
