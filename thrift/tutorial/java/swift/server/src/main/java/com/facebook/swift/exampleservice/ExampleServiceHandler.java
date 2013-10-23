package com.facebook.swift.exampleservice;

import com.facebook.swift.service.ThriftMethod;
import com.facebook.swift.service.ThriftService;

@ThriftService
public class ExampleServiceHandler
{
    private Runnable shutdownCallback;

    @ThriftMethod(value = "process_the_string")
    public ProcessedStringResults processString(String input) {
        return new ProcessedStringResults(new StringBuffer(input).reverse().toString(),
                                          input.toUpperCase(),
                                          input.toLowerCase());
    }

    @ThriftMethod
    public void sleep(int ms) throws InterruptedException
    {
        Thread.sleep(ms);
    }
}
