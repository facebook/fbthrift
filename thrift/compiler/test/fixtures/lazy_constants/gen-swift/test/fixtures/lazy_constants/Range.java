package test.fixtures.lazy_constants;

import com.facebook.swift.codec.*;
import com.facebook.swift.codec.ThriftField.Requiredness;
import com.facebook.swift.codec.ThriftField.Recursiveness;
import java.util.*;

import static com.google.common.base.MoreObjects.toStringHelper;

@ThriftStruct("Range")
public final class Range
{
    @ThriftConstructor
    public Range(
        @ThriftField(value=1, name="min", requiredness=Requiredness.REQUIRED) final int min,
        @ThriftField(value=2, name="max", requiredness=Requiredness.REQUIRED) final int max
    ) {
        this.min = min;
        this.max = max;
    }

    public static class Builder {
        private int min;

        public Builder setMin(int min) {
            this.min = min;
            return this;
        }
        private int max;

        public Builder setMax(int max) {
            this.max = max;
            return this;
        }

        public Builder() { }
        public Builder(Range other) {
            this.min = other.min;
            this.max = other.max;
        }

        public Range build() {
            return new Range (
                this.min,
                this.max
            );
        }
    }

    private final int min;

    @ThriftField(value=1, name="min", requiredness=Requiredness.REQUIRED)
    public int getMin() { return min; }

    private final int max;

    @ThriftField(value=2, name="max", requiredness=Requiredness.REQUIRED)
    public int getMax() { return max; }

    @Override
    public String toString()
    {
        return toStringHelper(this)
            .add("min", min)
            .add("max", max)
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

        Range other = (Range)o;

        return
            Objects.equals(min, other.min) &&
            Objects.equals(max, other.max);
    }

    @Override
    public int hashCode() {
        return Arrays.deepHashCode(new Object[] {
            min,
            max
        });
    }
}
