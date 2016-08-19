package test.fixtures.inheritance;

import com.facebook.swift.codec.*;
import com.facebook.swift.codec.ThriftField.Requiredness;
import com.facebook.swift.service.*;
import com.google.common.util.concurrent.ListenableFuture;
import java.io.*;
import java.util.*;

@ThriftService("MyRoot")
public interface MyRoot
{
    @ThriftService("MyRoot")
    public interface Async
    {
        @ThriftMethod(value = "do_root")
        ListenableFuture<Void> doRoot(
        );
    }
    @ThriftMethod(value = "do_root")
    void doRoot(
    );

}
