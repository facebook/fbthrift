package test.fixtures.module1;

import com.facebook.swift.codec.*;

public enum Enum
{
    ONE(1), TWO(2), THREE(3);

    private final int value;

    Enum(int value)
    {
        this.value = value;
    }

    @ThriftEnumValue
    public int getValue()
    {
        return value;
    }

    public static Enum fromInteger(int n) {
        switch (n) {
        case 1:
            return ONE;
        case 2:
            return TWO;
        case 3:
            return THREE;
        default:
            return null;
        }
    }
}
