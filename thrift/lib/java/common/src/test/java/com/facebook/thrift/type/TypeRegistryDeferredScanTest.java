/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.facebook.thrift.type;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.io.File;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLClassLoader;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import org.junit.jupiter.api.Test;

/**
 * Covers the {@code thrift.type-registry.defer-classpath-scan} path, which the rest of the suite
 * never reaches: every other test runs with the property unset, so they exercise the eager path
 * only and would all still pass if the deferred branch were broken.
 *
 * <p>Each test loads its own copy of TypeRegistry in an isolated class loader. That is not
 * incidental -- the flag is read in {@code <clinit>}, so a class already initialized by the test
 * JVM has baked in whatever the property was at that moment and cannot be re-tested. A fresh loader
 * is the only way to observe class-initialization behaviour more than once in one JVM.
 *
 * <p>The scan's effect is read directly off the private {@code hashList} field rather than inferred
 * from a lookup. Inferring would be weaker and, for these fixtures, unsafe: see the note on
 * recursion in {@link __fbthrift_TypeList_c0ffee11}. Lookups here use a prefix that matches
 * nothing, which exercises the trigger and returns cleanly.
 */
public class TypeRegistryDeferredScanTest {

  private static final String PROPERTY = "thrift.type-registry.defer-classpath-scan";
  private static final String TYPE_REGISTRY = "com.facebook.thrift.type.TypeRegistry";
  private static final String COUNTING_TYPE_LIST =
      "com.facebook.thrift.type.__fbthrift_TypeList_c0ffee11";

  /** A prefix no fixture maps to, so it drives the miss path without triggering a retry. */
  private static final String UNMATCHED_PREFIX = "ffffffff";

  /**
   * Child-first for {@code com.facebook.thrift.type} only.
   *
   * <p>Everything else -- netty, guava, slf4j, junit -- must come from the parent. Loading those
   * twice would give the isolated TypeRegistry a {@code ByteBuf} the test cannot pass arguments of,
   * and the failure would look like an unrelated type error rather than a loader problem.
   */
  private static final class TypePackageFirstLoader extends URLClassLoader {
    private static final String ISOLATED_PACKAGE = "com.facebook.thrift.type.";

    TypePackageFirstLoader(URL[] urls, ClassLoader parent) {
      super(urls, parent);
    }

    @Override
    protected Class<?> loadClass(String name, boolean resolve) throws ClassNotFoundException {
      synchronized (getClassLoadingLock(name)) {
        Class<?> loaded = findLoadedClass(name);
        if (loaded == null) {
          if (!name.startsWith(ISOLATED_PACKAGE)) {
            return super.loadClass(name, resolve);
          }
          loaded = findClass(name);
        }
        if (resolve) {
          resolveClass(loaded);
        }
        return loaded;
      }
    }
  }

  private static URL[] classpathUrls() throws MalformedURLException {
    String[] entries = System.getProperty("java.class.path").split(File.pathSeparator);
    List<URL> urls = new ArrayList<>(entries.length);
    for (String entry : entries) {
      if (!entry.isEmpty()) {
        urls.add(new File(entry).toURI().toURL());
      }
    }
    return urls.toArray(new URL[0]);
  }

  /**
   * Loads and initializes a fresh TypeRegistry with the property set as requested.
   *
   * <p>The property is set before loading and restored afterwards, so the flag is visible to {@code
   * <clinit>} and no test leaks it into the next one.
   */
  private static Class<?> freshTypeRegistry(boolean defer) throws Exception {
    String previous = System.getProperty(PROPERTY);
    if (defer) {
      System.setProperty(PROPERTY, "true");
    } else {
      System.clearProperty(PROPERTY);
    }
    try {
      ClassLoader loader =
          new TypePackageFirstLoader(
              classpathUrls(), TypeRegistryDeferredScanTest.class.getClassLoader());
      // initialize = true: the scan decision happens in <clinit>, so merely loading proves nothing.
      return Class.forName(TYPE_REGISTRY, true, loader);
    } finally {
      if (previous == null) {
        System.clearProperty(PROPERTY);
      } else {
        System.setProperty(PROPERTY, previous);
      }
    }
  }

  private static int hashListSize(Class<?> typeRegistry) throws Exception {
    Field field = typeRegistry.getDeclaredField("hashList");
    field.setAccessible(true);
    return ((Map<?, ?>) field.get(null)).size();
  }

  private static int scanCount(Class<?> typeRegistry) throws Exception {
    Class<?> counter = Class.forName(COUNTING_TYPE_LIST, true, typeRegistry.getClassLoader());
    return (Integer) counter.getMethod("instantiations").invoke(null);
  }

  private static Object findByHashPrefix(Class<?> typeRegistry, String hexPrefix) throws Exception {
    Method method = typeRegistry.getMethod("findByHashPrefix", String.class);
    try {
      return method.invoke(null, hexPrefix);
    } catch (InvocationTargetException e) {
      throw (Exception) e.getCause();
    }
  }

  @Test
  public void testScanRunsAtClassInitByDefault() throws Exception {
    Class<?> typeRegistry = freshTypeRegistry(false);
    assertTrue(
        hashListSize(typeRegistry) > 0,
        "default path must scan during class initialization, as it always has");
  }

  @Test
  public void testScanDeferredWhenPropertySet() throws Exception {
    Class<?> typeRegistry = freshTypeRegistry(true);
    assertEquals(
        0, hashListSize(typeRegistry), "with deferral on, class initialization must not scan");
    assertEquals(0, scanCount(typeRegistry), "no TypeList should have been instantiated yet");
  }

  @Test
  public void testDeferredScanRunsOnFirstLookupMiss() throws Exception {
    Class<?> typeRegistry = freshTypeRegistry(true);
    assertEquals(0, hashListSize(typeRegistry));

    assertNull(findByHashPrefix(typeRegistry, UNMATCHED_PREFIX));

    assertTrue(hashListSize(typeRegistry) > 0, "a hash-prefix miss must trigger the deferred scan");
    assertEquals(1, scanCount(typeRegistry));
  }

  @Test
  public void testDeferredScanRunsOnlyOnce() throws Exception {
    Class<?> typeRegistry = freshTypeRegistry(true);

    for (int i = 0; i < 5; i++) {
      assertNull(findByHashPrefix(typeRegistry, UNMATCHED_PREFIX));
    }

    // Each miss re-enters registered(), so without the holder's once-only initialization this
    // would be 5. A miss is not cached -- findByHashPrefix only populates its cache on success.
    assertEquals(1, scanCount(typeRegistry), "the holder must run the scan exactly once");
  }

  @Test
  public void testEagerPathDoesNotInitializeTheHolder() throws Exception {
    Class<?> typeRegistry = freshTypeRegistry(false);
    int afterInit = scanCount(typeRegistry);

    assertNull(findByHashPrefix(typeRegistry, UNMATCHED_PREFIX));

    // The guarded call in registered() is the only reference to DeferedInitialize, so with the
    // flag off it is never reached and the scan cannot run a second time.
    assertEquals(afterInit, scanCount(typeRegistry), "eager path must not scan twice");
    assertEquals(1, afterInit);
  }

  @Test
  public void testDeferredPathResolvesTheSameAsEager() throws Exception {
    // Same ambiguous prefix TypeRegistryTest uses, reached through the deferred path. The point
    // is that deferral changes when the index is built, not what it contains.
    Class<?> eager = freshTypeRegistry(false);
    Class<?> deferred = freshTypeRegistry(true);

    assertTrue(hashListSize(eager) > 0, "eager path has already scanned");
    assertEquals(0, hashListSize(deferred), "deferred path has not scanned yet");

    String eagerFailure = ambiguityFailure(eager);
    String deferredFailure = ambiguityFailure(deferred);

    assertNotNull(eagerFailure, "0577 is ambiguous across the test fixtures");
    assertEquals(eagerFailure, deferredFailure);
    assertEquals(
        hashListSize(eager),
        hashListSize(deferred),
        "deferral changes when the index is built, not what it contains");
  }

  /**
   * Returns the exception class name for the ambiguous prefix, or null if none was thrown.
   *
   * <p>Compared by name rather than by type: the isolated loader has its own
   * AmbiguousUniversalNameException, so {@code assertThrows} against this test's copy would not
   * match it.
   */
  private static String ambiguityFailure(Class<?> typeRegistry) throws Exception {
    try {
      findByHashPrefix(typeRegistry, "0577");
      return null;
    } catch (Exception e) {
      return e.getClass().getName();
    }
  }
}
