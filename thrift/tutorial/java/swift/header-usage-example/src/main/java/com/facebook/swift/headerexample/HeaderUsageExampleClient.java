package com.facebook.swift.headerexample;

import com.facebook.swift.service.ThriftMethod;
import com.facebook.swift.service.ThriftService;

@ThriftService
public interface HeaderUsageExampleClient extends AutoCloseable
{
    @ThriftMethod
    public void headerUsageExampleMethod();

    public void close();
}
