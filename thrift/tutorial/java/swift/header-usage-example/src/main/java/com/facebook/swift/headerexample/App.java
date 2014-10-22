package com.facebook.swift.headerexample;

import com.facebook.nifty.header.client.HeaderClientConnector;
import com.facebook.nifty.header.guice.HeaderServerModule;
import com.facebook.nifty.header.transport.THeaderTransport;
import com.facebook.swift.codec.guice.ThriftCodecModule;
import com.facebook.swift.fb303.EmptyOptionsSource;
import com.facebook.swift.fb303.Fb303ServiceModule;
import com.facebook.swift.fb303.OptionsSource;
import com.facebook.swift.service.ThriftClient;
import com.facebook.swift.service.ThriftClientManager;
import com.facebook.swift.service.ThriftServer;
import com.facebook.swift.service.guice.ThriftClientModule;
import com.facebook.swift.service.guice.ThriftServerModule;
import com.facebook.swift.service.guice.ThriftServerStatsModule;
import com.google.common.collect.ImmutableMap;
import com.google.common.net.HostAndPort;
import com.google.inject.*;
import io.airlift.bootstrap.LifeCycleManager;
import io.airlift.bootstrap.LifeCycleModule;
import io.airlift.configuration.ConfigurationFactory;
import io.airlift.configuration.ConfigurationModule;
import io.airlift.jmx.JmxModule;
import org.apache.thrift.protocol.TProtocol;
import org.apache.thrift.transport.TTransport;
import io.airlift.log.Logger;
import org.weakref.jmx.guice.MBeanModule;

import java.util.Map;
import java.util.concurrent.ExecutionException;

import static com.facebook.swift.fb303.Fb303ServiceExporter.fb303SourceBinder;
import static com.facebook.swift.service.guice.ThriftClientBinder.thriftClientBinder;
import static com.facebook.swift.service.guice.ThriftServiceExporter.thriftServerBinder;

public class App
{
    private static final Map<String, String> SERVER_CONFIGURATION = ImmutableMap.of(
            "thrift.transport", "header",
            "thrift.protocol", "header"
    );
    private static final Map<String, String> CLIENT_CONFIGURATION = ImmutableMap.<String, String>of();
    private static final Logger LOG = Logger.get(App.class);

    public static void main(String[] args) throws Exception {
        Injector serverInjector = Guice.createInjector(
                Stage.PRODUCTION,
                new ConfigurationModule(new ConfigurationFactory(SERVER_CONFIGURATION)),
                new LifeCycleModule(),
                new ThriftCodecModule(),
                new ThriftServerModule(),
                new ThriftServerStatsModule(),
                new Fb303ServiceModule(App.class),
                new MBeanModule(),
                new JmxModule(),
                new HeaderServerModule(),
                new AbstractModule() {
                    @Override
                    protected void configure() {
                        bind(OptionsSource.class).to(EmptyOptionsSource.class);

                        fb303SourceBinder(binder()).exportJmxUtils();
                        fb303SourceBinder(binder()).exportRuntimeMBeans();

                        bind(HeaderUsageExampleHandler.class).in(Scopes.SINGLETON);
                        thriftServerBinder(binder()).exportThriftService(HeaderUsageExampleHandler.class);
                    }
                }
        );

        Injector clientInjector = Guice.createInjector(
                Stage.PRODUCTION,
                new LifeCycleModule(),
                new ConfigurationModule(new ConfigurationFactory(CLIENT_CONFIGURATION)),
                new ThriftCodecModule(),
                new ThriftClientModule(),
                new AbstractModule() {
                    @Override
                    protected void configure() {
                        thriftClientBinder(binder()).bindThriftClient(HeaderUsageExampleClient.class);
                    }
                }
        );

        // Start server
        LifeCycleManager serverLifeCycleManager = serverInjector.getInstance(LifeCycleManager.class);
        serverLifeCycleManager.start();
        ThriftServer server = serverInjector.getInstance(ThriftServer.class);
        int serverPort = server.getPort();

        // Start client manager and send request
        LifeCycleManager clientLifeCycleManager = clientInjector.getInstance(LifeCycleManager.class);
        clientLifeCycleManager.start();
        sendClientRequest(clientInjector, serverPort);

        // Stop client manager
        clientLifeCycleManager.stop();

        // Stop server
        serverLifeCycleManager.stop();
    }

    private static void sendClientRequest(Injector clientInjector, int serverPort) throws InterruptedException, ExecutionException {
        ThriftClientManager clientManager = clientInjector.getInstance(ThriftClientManager.class);
        ThriftClient<HeaderUsageExampleClient> clientFactory = clientInjector.getInstance(Key.get(new TypeLiteral<ThriftClient<HeaderUsageExampleClient>>() {
        }));
        HeaderUsageExampleClient client = clientFactory.open(new HeaderClientConnector(HostAndPort.fromParts("localhost", serverPort))).get();

        TProtocol outputProtocol = clientManager.getOutputProtocol(client);
        TTransport outputTransport = outputProtocol.getTransport();
        if (outputTransport instanceof THeaderTransport) {
            LOG.info("adding headers to next client request");
            THeaderTransport headerTransport = (THeaderTransport) outputTransport;
            headerTransport.setHeader("header_from", "client");
        }
        else {
            LOG.info("output transport for client was not THeaderTransport, client cannot send headers");
        }

        client.headerUsageExampleMethod();

        TProtocol inputProtocol = clientManager.getInputProtocol(client);
        TTransport inputTransport = inputProtocol.getTransport();
        if (inputTransport instanceof THeaderTransport) {
            LOG.info("headers received from the server in the response:");
            THeaderTransport headerTransport = (THeaderTransport) outputTransport;
            for (String key : headerTransport.getReadHeaders().keySet()) {
                LOG.info("header '" + key + "' => '" + headerTransport.getReadHeaders().get(key) + "'");
            }
        }
        else {
            LOG.info("output transport for client was not THeaderTransport, client cannot send headers");
        }
    }
}
