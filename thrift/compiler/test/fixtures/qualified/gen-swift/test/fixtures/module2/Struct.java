package test.fixtures.module2;

import com.facebook.swift.codec.*;
import com.facebook.swift.codec.ThriftField.Requiredness;
import com.facebook.swift.codec.ThriftField.Recursiveness;
import java.util.*;

import static com.google.common.base.MoreObjects.toStringHelper;

@ThriftStruct("Struct")
public final class Struct
{
    @ThriftConstructor
    public Struct(
        @ThriftField(value=1, name="first", requiredness=Requiredness.NONE) final test.fixtures.module0.Struct first,
        @ThriftField(value=2, name="second", requiredness=Requiredness.NONE) final test.fixtures.module1.Struct second
    ) {
        this.first = first;
        this.second = second;
    }

    public static class Builder {
        private test.fixtures.module0.Struct first;

        public Builder setFirst(test.fixtures.module0.Struct first) {
            this.first = first;
            return this;
        }
        private test.fixtures.module1.Struct second;

        public Builder setSecond(test.fixtures.module1.Struct second) {
            this.second = second;
            return this;
        }

        public Builder() { }
        public Builder(Struct other) {
            this.first = other.first;
            this.second = other.second;
        }

        public Struct build() {
            return new Struct (
                this.first,
                this.second
            );
        }
    }

    private final test.fixtures.module0.Struct first;

    @ThriftField(value=1, name="first", requiredness=Requiredness.NONE)
    public test.fixtures.module0.Struct getFirst() { return first; }

    private final test.fixtures.module1.Struct second;

    @ThriftField(value=2, name="second", requiredness=Requiredness.NONE)
    public test.fixtures.module1.Struct getSecond() { return second; }

    @Override
    public String toString()
    {
        return toStringHelper(this)
            .add("first", first)
            .add("second", second)
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

        Struct other = (Struct)o;

        return
            Objects.equals(first, other.first) &&
            Objects.equals(second, other.second);
    }

    @Override
    public int hashCode() {
        return Arrays.deepHashCode(new Object[] {
            first,
            second
        });
    }
}
