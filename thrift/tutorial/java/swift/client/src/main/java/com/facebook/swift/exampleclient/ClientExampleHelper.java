package com.facebook.swift.exampleclient;

import com.facebook.nifty.client.FramedClientConnector;
import com.facebook.nifty.client.NiftyClientChannel;
import com.facebook.nifty.client.NiftyClientConnector;
import com.facebook.nifty.header.client.HeaderClientConnector;
import com.facebook.swift.service.ThriftClient;
import com.google.inject.Inject;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.annotation.PostConstruct;
import javax.inject.Named;
import java.util.concurrent.ExecutionException;

import static com.google.common.net.HostAndPort.fromParts;

/**
 * A helper to show off the usage of guice injection of ThriftClient instances
 */
public class ClientExampleHelper
{
    private static final Logger logger = LoggerFactory.getLogger(ClientExampleHelper.class);

    private final ThriftClient<ExampleService> thriftClient;
    private final String inputString;

    public ClientExampleHelper(ThriftClient<ExampleService> thriftClient,
                               String inputString) {
        this.thriftClient = thriftClient;
        this.inputString = inputString;
    }

    /**
     * Use the injected ThriftClient to create a client and invoke thrift calls against that
     * client
     */
    public void execute() throws ExecutionException, InterruptedException
    {
        try (ExampleService client = thriftClient.open(getConnector()).get())
        {
            executeWithClientAndInput(client, inputString);
        }
    }

    /**
     * Invoke thrift calls against a given ExampleService client interface
     */
    public static void executeWithClientAndInput(ExampleService client, String inputString)
            throws ExecutionException, InterruptedException
    {
        client.sleep(100);
        ProcessedStringResults output = client.processString(inputString).get();
        logger.info(String.format("Client call: input == \"%s\", output == \"%s\"",
                                  inputString,
                                  output.toString()));
    }

    public static NiftyClientConnector<? extends NiftyClientChannel> getConnector()
    {
        return new HeaderClientConnector(fromParts("localhost", 4567));
    }
}
