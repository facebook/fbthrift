package com.facebook.swift.headerexample;

import com.facebook.nifty.core.RequestContext;
import com.facebook.nifty.core.RequestContexts;
import com.facebook.nifty.header.transport.THeaderTransport;
import com.facebook.swift.service.ThriftMethod;
import com.facebook.swift.service.ThriftService;
import org.apache.thrift.transport.TTransport;
import io.airlift.log.Logger;

@ThriftService
public class HeaderUsageExampleHandler
{
    private static final Logger LOG = Logger.get(HeaderUsageExampleHandler.class);

    @ThriftMethod
    public void headerUsageExampleMethod() {
        RequestContext requestContext = RequestContexts.getCurrentContext();

        // Display headers sent by client
        TTransport inputTransport = requestContext.getInputProtocol().getTransport();
        if (inputTransport instanceof THeaderTransport) {
            LOG.info("headers received from the client in the request:");
            THeaderTransport headerTransport = (THeaderTransport) inputTransport;
            for (String key : headerTransport.getReadHeaders().keySet()) {
                LOG.info("header '" + key + "' => '" + headerTransport.getReadHeaders().get(key) + "'");
            }
        }

        // Send some headers back to the client
        TTransport outputTransport = requestContext.getOutputProtocol().getTransport();
        if (outputTransport instanceof THeaderTransport) {
            LOG.info("adding headers to the response");
            THeaderTransport headerTransport = (THeaderTransport) inputTransport;
            headerTransport.setHeader("header_from", "server");
        }
    }
}
