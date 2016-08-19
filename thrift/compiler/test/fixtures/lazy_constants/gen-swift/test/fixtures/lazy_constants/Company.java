package test.fixtures.lazy_constants;

import com.facebook.swift.codec.*;

public enum Company
{
    FACEBOOK(0), WHATSAPP(1), OCULUS(2), INSTAGRAM(3);

    private final int value;

    Company(int value)
    {
        this.value = value;
    }

    @ThriftEnumValue
    public int getValue()
    {
        return value;
    }
}
