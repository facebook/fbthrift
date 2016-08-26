package test.fixtures.module1;

import com.facebook.swift.codec.*;
import com.google.common.collect.*;
import java.util.*;

public final class Constants
{
    private Constants() {
    }

    public static final test.fixtures.module1.Struct c1 = ;

    public static final List<test.fixtures.module1.Enum> e1s = ImmutableList.<test.fixtures.module1.Enum>builder()
        .add(test.fixtures.module1.Enum.fromInteger(1))
        .add(test.fixtures.module1.Enum.fromInteger(3))
        .build();
}
