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

import java.util.Collections;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * Counts how many times TypeRegistry's classpath scan has run, so a test can assert the scan
 * happens exactly once rather than merely that it happened.
 *
 * <p>The scan instantiates every {@code __fbthrift_TypeList_*} it finds, so one instantiation of
 * this class is one scan. The counter is per class loader, which is what makes it usable: the
 * deferral tests load TypeRegistry in an isolated loader, so this fixture is loaded there too and
 * its count reflects only that loader's scans.
 *
 * <p>Contributes NO type mappings, deliberately. An entry here would land in {@code hashList} and
 * could collide with the prefixes {@link TypeRegistryTest} relies on; an empty list cannot. It also
 * avoids a sharper trap: {@code registered()} returns true after loading a mapped class, and {@code
 * findByHashPrefix()} then retries. For a real generated type that terminates, because the class
 * registers itself during initialization and the retry hits {@code hashMap}. For a mapping whose
 * class does not self-register, the retry misses again and recurses without end.
 */
public class __fbthrift_TypeList_c0ffee11 implements TypeList {

  private static final AtomicInteger INSTANTIATIONS = new AtomicInteger();

  public __fbthrift_TypeList_c0ffee11() {
    INSTANTIATIONS.incrementAndGet();
  }

  /** Read reflectively by the deferral tests, which hold this class in a different loader. */
  public static int instantiations() {
    return INSTANTIATIONS.get();
  }

  @Override
  public List<TypeList.TypeMapping> getTypes() {
    return Collections.emptyList();
  }
}
