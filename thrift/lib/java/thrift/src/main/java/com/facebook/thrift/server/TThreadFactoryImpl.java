package com.facebook.thrift.server;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.atomic.AtomicInteger;

/**
* This class is an implementation of the <i>ThreadFactory</i> interface. This
* is useful to give Java threads meaningful names which is useful when using
* a tool like JConsole.
*/

public class TThreadFactoryImpl implements ThreadFactory
{
    protected String id_;
    protected Long version_;
    protected ThreadGroup threadGroup_;
    protected final AtomicInteger threadNbr_ = new AtomicInteger(1);
    protected static final Map<String, Long> poolVersionByName =
      new HashMap<String, Long>();

    public TThreadFactoryImpl(String id)
    {
        SecurityManager sm = System.getSecurityManager();
        threadGroup_ = ( sm != null ) ? sm.getThreadGroup() : Thread.currentThread().getThreadGroup();
        Long lastVersion;
        synchronized(this) {
          if ((lastVersion = poolVersionByName.get(id)) == null) {
            lastVersion = Long.valueOf(-1);
          }
          poolVersionByName.put(id, lastVersion + 1);
        }
        version_ = lastVersion != null ? lastVersion + 1 : 0;
        id_ = id;
    }

    public Thread newThread(Runnable runnable)
    {
        String name = id_ + "-" + version_ +
            "-thread-" + threadNbr_.getAndIncrement();
        Thread thread = new Thread(threadGroup_, runnable, name);
        return thread;
    }
}
