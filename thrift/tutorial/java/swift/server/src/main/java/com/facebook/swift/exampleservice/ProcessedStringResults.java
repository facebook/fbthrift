package com.facebook.swift.exampleservice;

import com.facebook.swift.codec.ThriftConstructor;
import com.facebook.swift.codec.ThriftField;
import com.facebook.swift.codec.ThriftStruct;

@ThriftStruct
public class ProcessedStringResults
{
    private final String reversedString;
    private final String uppercaseString;
    private final String lowercaseString;

    @ThriftConstructor
    public ProcessedStringResults(
            @ThriftField(value = 1) String reversedString,
            @ThriftField(value = 2) String uppercaseString,
            @ThriftField(value = 3) String lowercaseString)
    {
        this.reversedString = reversedString;
        this.uppercaseString = uppercaseString;
        this.lowercaseString = lowercaseString;
    }

    @ThriftField(value = 1)
    public String getReversedString()
    {
        return reversedString;
    }

    @ThriftField(value = 2)
    public String getUppercaseString()
    {
        return uppercaseString;
    }

    @ThriftField(value = 3)
    public String getLowercaseString()
    {
        return lowercaseString;
    }

    @Override
    public String toString()
    {
        return String.format("{ reverse: %s, upcase: %s, downcase: %s }",
                             reversedString,
                             uppercaseString,
                             lowercaseString);
    }
}
