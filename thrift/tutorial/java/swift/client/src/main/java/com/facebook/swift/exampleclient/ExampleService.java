package com.facebook.swift.exampleclient;

import com.facebook.swift.service.ThriftMethod;
import com.facebook.swift.service.ThriftService;
import com.google.common.util.concurrent.ListenableFuture;

@ThriftService
public interface ExampleService extends AutoCloseable
{
    public void close();

    @ThriftMethod(value = "process_the_string")
    public ListenableFuture<ProcessedStringResults> processString(String input);

    @ThriftMethod
    public void sleep(int i);
}
