package test.fixtures.optionals;

import com.facebook.swift.codec.*;

public enum Animal
{
    DOG(1), CAT(2), TARANTULA(3);

    private final int value;

    Animal(int value)
    {
        this.value = value;
    }

    @ThriftEnumValue
    public int getValue()
    {
        return value;
    }

    public static Animal fromInteger(int n) {
        switch (n) {
        case 1:
            return DOG;
        case 2:
            return CAT;
        case 3:
            return TARANTULA;
        default:
            return null;
        }
    }
}
