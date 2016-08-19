package test.fixtures.basic;

import com.facebook.swift.codec.*;

public enum MyEnum
{
    MY_VALUE1(0), MY_VALUE2(1);

    private final int value;

    MyEnum(int value)
    {
        this.value = value;
    }

    @ThriftEnumValue
    public int getValue()
    {
        return value;
    }
}
