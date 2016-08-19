package test.fixtures.lazy_constants;

import com.facebook.swift.codec.*;

public enum City
{
    NYC(0), MPK(1), SEA(2), LON(3);

    private final int value;

    City(int value)
    {
        this.value = value;
    }

    @ThriftEnumValue
    public int getValue()
    {
        return value;
    }
}
