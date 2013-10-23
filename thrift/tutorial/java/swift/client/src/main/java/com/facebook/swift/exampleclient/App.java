package com.facebook.swift.exampleclient;

import com.facebook.swift.codec.guice.ThriftCodecModule;
import com.facebook.swift.fb303.Fb303;
import com.facebook.swift.service.ThriftClient;
import com.facebook.swift.service.ThriftClientManager;
import com.facebook.swift.service.guice.ThriftClientModule;
import com.google.common.base.Joiner;
import com.google.common.collect.ImmutableMap;
import com.google.inject.*;
import com.google.inject.name.Names;
import com.mycila.inject.jsr250.Jsr250;
import com.mycila.inject.jsr250.Jsr250Injector;
import io.airlift.bootstrap.LifeCycleManager;
import io.airlift.bootstrap.LifeCycleModule;
import io.airlift.configuration.ConfigurationFactory;
import io.airlift.configuration.ConfigurationModule;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.*;
import java.util.concurrent.ExecutionException;

import static com.facebook.swift.service.guice.ThriftClientBinder.thriftClientBinder;

public class App
{
    private static final Logger LOGGER = LoggerFactory.getLogger(App.class);

    public static void main(String[] args)
            throws Exception
    {
        new App().run();
    }

    public void run() throws Exception
    {
        // Make a client call using various different client setup mechanisms
        plainClientExample();
        thriftClientFactoryExample();
        guiceLifecycleExample();
        jsr250InjectorExample();

        collectCounters();

        // Shutdown server
        shutdownServer();
    }

    /**
     * An example that manually creates a {@link ThriftClientManager} and from that manager
     * manually creates and connects the client.
     *
     * {@link ClientExampleHelper} is used only to make the actually thrift method calls
     */
    private static void plainClientExample() throws ExecutionException, InterruptedException
    {
        try (ThriftClientManager clientManager = new ThriftClientManager();
             ExampleService rawClient = clientManager.createClient(ClientExampleHelper.getConnector(),
                                                                    ExampleService.class).get())
        {
            ClientExampleHelper.executeWithClientAndInput(rawClient, "plainClientExample");
        }
    }

    /**
     * An example that manually creates a {@link ThriftClientManager} and a {@link ThriftClient}
     * but lets the {@link ClientExampleHelper} open the connection
     */
    private static void thriftClientFactoryExample() throws ExecutionException, InterruptedException
    {
        try (ThriftClientManager clientManager = new ThriftClientManager())
        {
            ThriftClient<ExampleService> thriftClient = new ThriftClient<>(clientManager, ExampleService.class);
            new ClientExampleHelper(thriftClient, "thriftClientFactoryExample").execute();
        }
    }

    /**
     * An example that uses the standard guice injector, and a {@link LifeCycleManager} to manage
     * lifetimes of injected instances.
     *
     * Guice configuration handles creation of the {@link ThriftClient}
     */
    private static void guiceLifecycleExample() throws Exception
    {
        ImmutableMap<String, String> configuration = ImmutableMap.of(
                "ExampleService.thrift.client.read-timeout", "1000ms"
        );
        Injector injector = Guice.createInjector(
                Stage.PRODUCTION,
                new LifeCycleModule(),
                new ConfigurationModule(new ConfigurationFactory(configuration)),
                new ThriftCodecModule(),
                new ThriftClientModule(),
                new Module()
                {
                    @Override
                    public void configure(Binder binder)
                    {
                        binder.bind(String.class)
                              .annotatedWith(Names.named("stringToProcess"))
                              .toInstance("guiceLifecycleExample");
                        thriftClientBinder(binder).bindThriftClient(ExampleService.class);
                    }
                });

        LifeCycleManager manager = injector.getInstance(LifeCycleManager.class);
        manager.start();

        try {
            ThriftClient<ExampleService> clientFactory =
                    injector.getInstance(Key.get(new TypeLiteral<ThriftClient<ExampleService>>() {}));
            String stringToProcess =
                    injector.getInstance(Key.get(String.class, Names.named("stringToProcess")));

            new ClientExampleHelper(clientFactory, stringToProcess).execute();
        }
        finally {
            manager.stop();
        }
    }

    /**
     * An example that uses the Mycila Guice injector (with JSR250 extensions for controlling
     * lifetimes of injected instances)
     *
     * Guice configuration handles creation of the {@link ThriftClient}
     */
    private static void jsr250InjectorExample() throws ExecutionException, InterruptedException
    {
        ImmutableMap<String, String> configuration = ImmutableMap.of(
                "ExampleService.thrift.client.read-timeout", "1000ms"
        );

        // In this case, no child injector is needed because Jsr250 handles post-construct
        // exceptions gracefully by running the @PreDestroy for any injected instances
        Jsr250Injector injector = Jsr250.createInjector(
                Stage.PRODUCTION,
                new ConfigurationModule(new ConfigurationFactory(configuration)),
                new ThriftCodecModule(),
                new ThriftClientModule(),
                new Module()
                {
                    @Override
                    public void configure(Binder binder)
                    {
                        binder.bind(String.class)
                              .annotatedWith(Names.named("stringToProcess"))
                              .toInstance("jsr250InjectorExample");
                        thriftClientBinder(binder).bindThriftClient(ExampleService.class);
                    }
                });

        ThriftClient<ExampleService> clientFactory =
                injector.getInstance(Key.get(new TypeLiteral<ThriftClient<ExampleService>>() {}));
        String stringToProcess =
                injector.getInstance(Key.get(String.class, Names.named("stringToProcess")));
        new ClientExampleHelper(clientFactory, stringToProcess).execute();

        injector.destroy();
    }

    private static void collectCounters() throws ExecutionException, InterruptedException
    {
        try (ThriftClientManager clientManager = new ThriftClientManager();
             Fb303 rawClient = clientManager.createClient(ClientExampleHelper.getConnector(), Fb303.class).get())
        {
            LOGGER.info("fb303 counters:\n{}", Joiner.on("\n").join(new TreeMap<>(rawClient.getCounters()).entrySet()));
        }
    }

    private static void shutdownServer() throws ExecutionException, InterruptedException
    {
        try (ThriftClientManager clientManager = new ThriftClientManager();
             Fb303 rawClient = clientManager.createClient(ClientExampleHelper.getConnector(), Fb303.class).get())
        {
            rawClient.shutdown();
        }
    }
}
