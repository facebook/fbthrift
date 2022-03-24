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

import com.google.common.annotations.VisibleForTesting;
import io.netty.buffer.ByteBuf;
import io.netty.buffer.ByteBufUtil;
import io.netty.buffer.Unpooled;
import java.util.HashMap;
import java.util.Map;
import java.util.TreeMap;
import java.util.concurrent.ConcurrentHashMap;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public final class TypeRegistry {
  private static final Logger LOGGER = LoggerFactory.getLogger(TypeRegistry.class);

  /** Sorted map for universal name hash to Type */
  private static final TreeMap<ByteBuf, Type> hashMap = new TreeMap<>();

  /** Local cache for sorted map. */
  private static final Map<ByteBuf, Type> cache = new ConcurrentHashMap<>();

  /** Hash map for class name to Type */
  private static final Map<Class, Type> classMap = new HashMap<>();

  private TypeRegistry() {}

  /**
   * Validate given universal name and the class. They should not be associated with another object.
   * If universal name or class is already associated with another entry, IllegalArgumentException
   * is thrown.
   *
   * @param universalName
   * @param clazz
   */
  private static void validate(UniversalName universalName, Class clazz) {
    if (exist(universalName) && hashMap.get(universalName.getHashBytes()).getClazz() != clazz) {
      String msg =
          "Universal name already registered with another class "
              + hashMap.get(universalName.getHashBytes()).getClazz().getCanonicalName();
      LOGGER.error(msg);
      throw new IllegalArgumentException(msg);
    }
    if (exist(clazz)
        && !classMap
            .get(clazz)
            .getUniversalName()
            .getHashBytes()
            .equals(universalName.getHashBytes())) {
      String msg =
          "Class name already registered with another universal name "
              + classMap.get(clazz).getUniversalName().getUri();
      LOGGER.error(msg);
      throw new IllegalArgumentException(msg);
    }
  }

  /**
   * Add the given universal name and class to the registry. If universal name or class is already
   * associated with another entry, IllegalArgumentException is thrown.
   *
   * @param universalName UniversalName associated with the class
   * @param clazz Class name associated with the UniversalName
   * @return Hash bytes as hexadecimal value.
   */
  public static String add(UniversalName universalName, Class clazz) {
    validate(universalName, clazz);
    LOGGER.info("Adding universal name ? and class ?", universalName, clazz.getName());

    Type type = new Type(universalName, clazz);
    hashMap.put(universalName.getHashBytes(), type);
    classMap.put(clazz, type);

    return universalName.getHash();
  }

  private static boolean startsWith(ByteBuf bytes, ByteBuf prefix) {
    for (int i = 0; i < Math.min(prefix.capacity(), bytes.capacity()); i++) {
      if (prefix.getByte(i) != bytes.getByte(i)) {
        return false;
      }
    }
    return prefix.capacity() <= bytes.capacity();
  }

  /**
   * Find Type by given hash prefix. For example, if a Type is registered with a hash
   * "af420267bd92e0cb", it can be returned by providing any hash prefix, like "af42", "af4202" etc
   * if they return exactly one Type.
   *
   * @param prefix prefix as byte array
   * @return Type
   * @throws AmbiguousUniversalNameException if the prefix return more than one Type.
   */
  public static Type findByHashPrefix(ByteBuf prefix) throws AmbiguousUniversalNameException {
    if (cache.containsKey(prefix)) {
      return cache.get(prefix);
    }

    ByteBuf key = hashMap.ceilingKey(prefix);
    if (key == null || !startsWith(key, prefix)) {
      return null;
    }

    ByteBuf nextKey = hashMap.higherKey(key);
    if (nextKey == null || !startsWith(nextKey, prefix)) {
      Type type = hashMap.get(key);
      cache.put(prefix, type);
      return type;
    }

    throw new AmbiguousUniversalNameException(ByteBufUtil.hexDump(prefix));
  }

  /**
   * Find Type by given hash prefix. Same as the {@link #findByHashPrefix(ByteBuf) findByHashPrefix}
   * but taking prefix as hexadecimal String.
   *
   * @param hexPrefix as hexadecimal string
   * @return Type
   */
  public static Type findByHashPrefix(String hexPrefix) throws AmbiguousUniversalNameException {
    return findByHashPrefix(Unpooled.wrappedBuffer(ByteBufUtil.decodeHexDump(hexPrefix)));
  }

  /**
   * Find Type by given class name.
   *
   * @param clazz
   * @return Type
   */
  public static Type findByClass(Class clazz) {
    return classMap.get(clazz);
  }

  /**
   * Find Type by given universal name.
   *
   * @param universalName Universal name
   * @return Type
   */
  public static Type findByUniversalName(UniversalName universalName) {
    return hashMap.get(universalName.getHashBytes());
  }

  /**
   * Check if the class is already in the registry.
   *
   * @param clazz Name of the class
   * @return True if already in the registry, false otherwise
   */
  public static boolean exist(Class clazz) {
    return classMap.containsKey(clazz);
  }

  /**
   * Check if the universal name is already in the registry.
   *
   * @param universalName Universal name
   * @return True if already in the registry, false otherwise
   */
  public static boolean exist(UniversalName universalName) {
    return hashMap.containsKey(universalName.getHashBytes());
  }

  @VisibleForTesting
  public static int size() {
    return hashMap.size();
  }

  @VisibleForTesting
  public static void clear() {
    LOGGER.info("Clearing type registry");
    hashMap.clear();
    classMap.clear();
  }
}
