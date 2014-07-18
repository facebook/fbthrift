package com.facebook.swift.exampleservice;

import com.facebook.nifty.codec.ThriftFrameCodecFactory;
import com.facebook.nifty.core.NiftyTimer;
import com.facebook.nifty.core.NiftyNoOpSecurityFactory;
import com.facebook.nifty.duplex.TDuplexProtocolFactory;
import com.facebook.nifty.header.codec.HeaderThriftCodecFactory;
import com.facebook.nifty.header.guice.HeaderServerModule;
import com.facebook.nifty.header.protocol.TDuplexHeaderProtocolFactory;
import com.facebook.swift.codec.ThriftCodecManager;
import com.facebook.swift.codec.guice.ThriftCodecModule;
import com.facebook.swift.fb303.*;
import com.facebook.swift.service.ThriftEventHandler;
import com.facebook.swift.service.ThriftServer;
import com.facebook.swift.service.ThriftServerConfig;
import com.facebook.swift.service.ThriftServiceProcessor;
import com.facebook.swift.service.guice.ThriftServerModule;
import com.facebook.swift.service.guice.ThriftServerStatsModule;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableSet;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.AbstractModule;
import com.google.inject.Scopes;
import com.google.inject.Stage;
import io.airlift.bootstrap.LifeCycleManager;
import io.airlift.bootstrap.LifeCycleModule;
import io.airlift.configuration.ConfigurationFactory;
import io.airlift.configuration.ConfigurationModule;
import io.airlift.jmx.JmxModule;
import org.jboss.netty.logging.InternalLoggerFactory;
import org.jboss.netty.logging.Slf4JLoggerFactory;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.weakref.jmx.guice.MBeanModule;

import javax.inject.Inject;
import java.lang.management.ManagementFactory;
import java.util.Map;
import java.util.concurrent.ExecutorService;

import static com.facebook.swift.fb303.Fb303ServiceExporter.fb303SourceBinder;
import static com.facebook.swift.service.guice.ThriftServiceExporter.thriftServerBinder;

public class App
{
    private static final Logger LOGGER = LoggerFactory.getLogger(App.class);
    private static final ImmutableMap<String,String> SERVER_CONFIGURATION = ImmutableMap.of(
            "thrift.port", "4567",
            "thrift.threads.max", "200",
            "thrift.transport", "header",
            "thrift.protocol", "header"
    );

    public static void main(String[] args) throws Exception
    {
        new App().run();
    }

    public void run() throws Exception
    {
        InternalLoggerFactory.setDefaultFactory(new Slf4JLoggerFactory());

        runPlainServer();
        //runGuiceServer();
    }

    /**
     * Start a {@link ThriftServer} without using Guice dependency injection
     */
    public void runPlainServer() throws Exception
    {
        ThriftServerConfig config =
                new ThriftServerConfig().setWorkerThreads(200)
                                        .setPort(4567)
                                        .setTransportName("header")
                                        .setProtocolName("header");

        ThriftCodecManager codecManager = new ThriftCodecManager();

        ServiceInfo serviceInfo = new ServiceInfo("ExampleService", "1.0");

        // Fill this in to customize your service's response to
        // fb303 getOptions()
        OptionsSource optionsSource = new EmptyOptionsSource();

        ImmutableSet.Builder<Fb303Source> sourceSetBuilder = new ImmutableSet.Builder<>();

        // Add fb303 stats from the JVM
        MBeanSourceAdapter standardBeansSource = MBeanSourceAdapter.STANDARD_MBEANS_ADAPTER;
        standardBeansSource.setMBeanServer(ManagementFactory.getPlatformMBeanServer());
        sourceSetBuilder.add(standardBeansSource);

        // Add custom stats source (fill this in with your server's stats)
        sourceSetBuilder.add(new StatsSourceAdapter(new ExampleStatsSource()));

        Fb303Service fb303Handler = new Fb303Service(codecManager,
                                                     serviceInfo,
                                                     optionsSource,
                                                     sourceSetBuilder.build());

        ExampleServiceHandler exampleServiceHandler = new ExampleServiceHandler();

        ThriftServiceProcessor processor =
          new ThriftServiceProcessor(codecManager,
                                     ImmutableList.<ThriftEventHandler>of(),
                                     exampleServiceHandler,
                                     fb303Handler);
        final ThriftServer server =
                new ThriftServer(
                        processor,
                        config,
                        new NiftyTimer("thrift"),
                        ImmutableMap.<String, ThriftFrameCodecFactory>of("header", new HeaderThriftCodecFactory()),
                        ImmutableMap.<String, TDuplexProtocolFactory>of("header", new TDuplexHeaderProtocolFactory()),
                        ImmutableMap.<String, ExecutorService>of(),
                        new NiftyNoOpSecurityFactory());

        fb303Handler.setShutdownHandler(new ServerShutdownHandler(server));
        server.start();
    }

    /**
     * Start a {@link ThriftServer} using Guice
     */
    public void runGuiceServer() throws Exception
    {
        Injector injector = Guice.createInjector(
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
                new AbstractModule()
                {
                    @Override
                    public void configure()
                    {
                        bind(Runnable.class).annotatedWith(Fb303ShutdownHandler.class).to(LifeCycleShutdownHandler.class);

                        bind(OptionsSource.class).to(EmptyOptionsSource.class);

                        bind(StatsSource.class).to(ExampleStatsSource.class);
                        fb303SourceBinder(binder()).exportStats();
                        fb303SourceBinder(binder()).exportJmxUtils();
                        fb303SourceBinder(binder()).exportRuntimeMBeans();

                        bind(ExampleServiceHandler.class).in(Scopes.SINGLETON);
                        thriftServerBinder(binder()).exportThriftService(ExampleServiceHandler.class);
                    }
                });

        // Inject the lifecycle into this instance, so we can use it for shutdown
        LifeCycleManager lifeCycleManager = injector.getInstance(LifeCycleManager.class);
        lifeCycleManager.start();
    }

    /**
     * A runnable handler for shutting down a {@link ThriftServer} directly
     */
    private static class ServerShutdownHandler implements Runnable
    {
        private final ThriftServer server;

        @Inject
        public ServerShutdownHandler(ThriftServer server)
        {
            this.server = server;
        }

        @Override
        public void run()
        {
            server.close();
        }
    }

    /**
     * A runnable handler for shutting down a {@link com.facebook.swift.service.ThriftServer} via a {@link io.airlift.bootstrap.LifeCycleManager}
     */
    public static class LifeCycleShutdownHandler implements Runnable
    {
        private final Injector injector;

        @Inject
        public LifeCycleShutdownHandler(Injector injector)
        {
            this.injector = injector;
        }

        @Override
        public void run()
        {
            LifeCycleManager lifeCycleManager = injector.getInstance(LifeCycleManager.class);
            try {
                lifeCycleManager.stop();
            } catch (Exception e) {
                LOGGER.warn("Failed to stop LifeCycleManager");
            }
        }
    }

    // Fill this in to add custom counters and exported values (in addition to
    // automatically generated thrift counters from swift)
    private static class ExampleStatsSource implements StatsSource
    {
        public Map<String, Long> getCounters() {
            return ImmutableMap.of();
        }

        public Map<String, String> getAttributes() {
            return ImmutableMap.of();
        }

    }
}
