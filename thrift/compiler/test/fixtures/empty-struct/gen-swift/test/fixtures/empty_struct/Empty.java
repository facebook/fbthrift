package test.fixtures.empty_struct;

import com.facebook.swift.codec.*;
import com.facebook.swift.codec.ThriftField.Requiredness;
import com.facebook.swift.codec.ThriftField.Recursiveness;
import java.util.*;

import static com.google.common.base.MoreObjects.toStringHelper;

@ThriftStruct("Empty")
public final class Empty
{
    @ThriftConstructor
    public Empty(
    ) {
    }

    public static class Builder {

        public Builder() { }
        public Builder(Empty other) {
        }

        public Empty build() {
            return new Empty (
            );
        }
    }

    @Override
    public String toString()
    {
        return toStringHelper(this)
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

        Empty other = (Empty)o;

        return
            true;
    }

    @Override
    public int hashCode() {
        return Arrays.deepHashCode(new Object[] {
        });
    }
}
