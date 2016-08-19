package test.fixtures.exceptions;

import com.facebook.swift.codec.*;
import com.facebook.swift.codec.ThriftField.Requiredness;
import com.facebook.swift.codec.ThriftField.Recursiveness;
import java.util.*;

@ThriftStruct("Banal")
public final class Banal extends Exception
{
    private static final long serialVersionUID = 1L;

    @ThriftConstructor
    public Banal(
    ) {
    }

    public static class Builder {

        public Builder() { }
        public Builder(Banal other) {
        }

        public Banal build() {
            return new Banal (
            );
        }
    }
}
