/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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

package com.facebook.thrift;

import com.facebook.thrift.java.test.StructWithAllTypes;
import com.facebook.thrift.java.test.StructWithMaps;
import com.facebook.thrift.protocol.TCompactJSONProtocol;
import com.facebook.thrift.protocol.TProtocol;
import com.facebook.thrift.transport.TIOStreamTransport;
import com.facebook.thrift.transport.TMemoryInputTransport;
import com.facebook.thrift.utils.StandardCharsets;
import java.io.ByteArrayOutputStream;
import java.util.HashMap;
import org.junit.Test;

public class TCompactJSONTest extends junit.framework.TestCase {

  @Test
  public void testCompactJSONProtocol() throws Exception {
    final ByteArrayOutputStream bos = new ByteArrayOutputStream();
    final TIOStreamTransport out = new TIOStreamTransport(bos);
    final TProtocol oproto = new TCompactJSONProtocol(out);

    final StructWithAllTypes struct = new StructWithAllTypes.Builder().setBb(true).build();
    struct.write(oproto);

    final String json = new String(bos.toByteArray(), StandardCharsets.UTF_8);

    TMemoryInputTransport in = new TMemoryInputTransport(json.getBytes(StandardCharsets.UTF_8));
    TProtocol iproto = new TCompactJSONProtocol(in);

    StructWithAllTypes read0 = new StructWithAllTypes();
    read0.read(iproto);
    assertEquals(struct, read0);

    // Test backward compatibility (i.e. boolean serialized as 0/1)
    String oldJson = json.replaceAll("true", "1");
    in = new TMemoryInputTransport(oldJson.getBytes(StandardCharsets.UTF_8));
    iproto = new TCompactJSONProtocol(in);

    StructWithAllTypes read1 = new StructWithAllTypes();
    read1.read(iproto);
    assertEquals(struct, read1);
  }

  @Test
  public void testCompactJSONWithMaps() throws Exception {
    final ByteArrayOutputStream bos = new ByteArrayOutputStream();
    final TIOStreamTransport out = new TIOStreamTransport(bos);
    final TProtocol oproto = new TCompactJSONProtocol(out);

    final StructWithMaps struct =
        new StructWithMaps.Builder()
            .setStringstrings(
                new HashMap<String, String>() {
                  {
                    put("1", "one");
                  }
                })
            .setBoolstrings(
                new HashMap<Boolean, String>() {
                  {
                    put(true, "VRAI");
                  }
                })
            .setStringbools(
                new HashMap<String, Boolean>() {
                  {
                    put("FAUX", false);
                  }
                })
            .setIntstrings(
                new HashMap<Integer, String>() {
                  {
                    put(2, "two");
                  }
                })
            .setStringints(
                new HashMap<String, Integer>() {
                  {
                    put("three", 3);
                  }
                })
            .build();
    struct.write(oproto);

    String json = new String(bos.toByteArray(), StandardCharsets.UTF_8);
    TMemoryInputTransport in = new TMemoryInputTransport(json.getBytes(StandardCharsets.UTF_8));
    TProtocol iproto = new TCompactJSONProtocol(in);

    StructWithMaps read0 = new StructWithMaps();
    read0.read(iproto);
    assertEquals(struct, read0);

    // Test backward compatibility (i.e. boolean serialized as 0/1)
    json = json.replaceAll("true", "\"1\""); // true is a map key here and was generated with quotes
    json = json.replaceAll("false", "0");
    in = new TMemoryInputTransport(json.getBytes(StandardCharsets.UTF_8));
    iproto = new TCompactJSONProtocol(in);

    StructWithMaps read1 = new StructWithMaps();
    read1.read(iproto);
    assertEquals(struct, read1);
  }
}
