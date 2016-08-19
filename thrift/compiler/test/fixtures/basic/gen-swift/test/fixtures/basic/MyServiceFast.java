package test.fixtures.basic;

import com.facebook.swift.codec.*;
import com.facebook.swift.codec.ThriftField.Requiredness;
import com.facebook.swift.service.*;
import com.google.common.util.concurrent.ListenableFuture;
import java.io.*;
import java.util.*;

@ThriftService("MyServiceFast")
public interface MyServiceFast
{
    @ThriftService("MyServiceFast")
    public interface Async
    {
        @ThriftMethod(value = "ping")
        ListenableFuture<Void> ping(
        );

        @ThriftMethod(value = "getRandomData")
        ListenableFuture<String> getRandomData(
        );

        @ThriftMethod(value = "hasDataById")
        ListenableFuture<Boolean> hasDataById(
            @ThriftField(value=1, name="id", requiredness=Requiredness.NONE) final long id
        );

        @ThriftMethod(value = "getDataById")
        ListenableFuture<String> getDataById(
            @ThriftField(value=1, name="id", requiredness=Requiredness.NONE) final long id
        );

        @ThriftMethod(value = "putDataById")
        ListenableFuture<Void> putDataById(
            @ThriftField(value=1, name="id", requiredness=Requiredness.NONE) final long id,
            @ThriftField(value=2, name="data", requiredness=Requiredness.NONE) final String data
        );

        @ThriftMethod(value = "lobDataById",
                      oneway = true)
        ListenableFuture<Void> lobDataById(
            @ThriftField(value=1, name="id", requiredness=Requiredness.NONE) final long id,
            @ThriftField(value=2, name="data", requiredness=Requiredness.NONE) final String data
        );
    }
    @ThriftMethod(value = "ping")
    void ping(
    );


    @ThriftMethod(value = "getRandomData")
    String getRandomData(
    );


    @ThriftMethod(value = "hasDataById")
    boolean hasDataById(
        @ThriftField(value=1, name="id", requiredness=Requiredness.NONE) final long id
    );


    @ThriftMethod(value = "getDataById")
    String getDataById(
        @ThriftField(value=1, name="id", requiredness=Requiredness.NONE) final long id
    );


    @ThriftMethod(value = "putDataById")
    void putDataById(
        @ThriftField(value=1, name="id", requiredness=Requiredness.NONE) final long id,
        @ThriftField(value=2, name="data", requiredness=Requiredness.NONE) final String data
    );


    @ThriftMethod(value = "lobDataById",
                  oneway = true)
    void lobDataById(
        @ThriftField(value=1, name="id", requiredness=Requiredness.NONE) final long id,
        @ThriftField(value=2, name="data", requiredness=Requiredness.NONE) final String data
    );

}
