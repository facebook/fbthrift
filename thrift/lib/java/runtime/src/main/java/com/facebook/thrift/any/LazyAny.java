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

package com.facebook.thrift.any;

import com.facebook.thrift.payload.ThriftSerializable;
import com.facebook.thrift.payload.Writer;
import com.facebook.thrift.protocol.ByteBufTProtocol;
import com.facebook.thrift.type.HashAlgorithmSHA256;
import com.facebook.thrift.type.Type;
import com.facebook.thrift.type.TypeRegistry;
import com.facebook.thrift.type.UniversalName;
import com.facebook.thrift.util.SerializationProtocol;
import com.facebook.thrift.util.SerializerUtil;
import com.facebook.thrift.util.resources.RpcResources;
import com.google.common.base.Strings;
import io.netty.buffer.ByteBuf;
import java.util.Objects;
import java.util.function.Function;
import org.apache.thrift.conformance.Any;
import org.apache.thrift.conformance.StandardProtocol;

/** LazyAny implementation that contains Any object. Any is created via Builder object. */
public abstract class LazyAny<T> {
  public abstract <T> T get();

  public abstract Any getAny();

  @Override
  public final int hashCode() {
    Any any = getAny();

    if (any == null) {
      return 0;
    }

    return any.hashCode();
  }

  @Override
  public final boolean equals(Object obj) {
    if (obj == this) {
      return true;
    }

    if (!(obj instanceof LazyAny)) {
      return false;
    }

    Any any = getAny();
    if (any == null) {
      return false;
    }

    LazyAny other = (LazyAny) obj;
    return any.equals(other.getAny());
  }

  public static class Builder<T> {
    private T value;

    private String customProtocol;

    private SerializationProtocol serializationProtocol = SerializationProtocol.TCompact;

    private Function<Object, ByteBuf> serializer;

    private boolean useHashPrefix;

    private boolean useUri;

    private int numberOfBytes = HashAlgorithmSHA256.INSTANCE.getMinHashBytes();

    public Builder() {}

    public Builder(T value) {
      this.value = value;
    }

    public Builder<T> setValue(T value) {
      this.value = value;
      return this;
    }

    public Builder<T> setProtocol(SerializationProtocol serializationProtocol) {
      this.serializationProtocol = serializationProtocol;
      return this;
    }

    public Builder<T> setCustomProtocol(String customProtocol) {
      this.customProtocol = customProtocol;
      return this;
    }

    public Builder<T> useHashPrefix() {
      this.useHashPrefix = true;
      return this;
    }

    public Builder<T> useHashPrefix(int numberOfBytes) {
      this.useHashPrefix = true;
      this.numberOfBytes = numberOfBytes;
      return this;
    }

    public Builder<T> useUri() {
      this.useUri = true;
      return this;
    }

    public Builder<T> setSerializer(Function<Object, ByteBuf> serializer) {
      this.serializer = serializer;
      return this;
    }

    public LazyAny build() {
      Objects.requireNonNull(value, "Must have a value");
      Type type = TypeRegistry.findByClass(value.getClass());
      if (type == null) {
        throw new IllegalArgumentException("No type found for value " + value.getClass());
      }

      if (useHashPrefix && useUri) {
        throw new IllegalStateException("Must set either the useHashPrefix or useUri, not both");
      }

      if (!useHashPrefix && !useUri) {
        UniversalName un = type.getUniversalName();
        if (un.preferHash(numberOfBytes)) {
          useHashPrefix = true;
        } else {
          useUri = true;
        }
      }

      if (numberOfBytes < 1) {
        throw new IllegalArgumentException("When using hash prefix must select one or more bytes");
      }

      if (!Strings.isNullOrEmpty(customProtocol) && serializer == null) {
        throw new IllegalArgumentException(
            "When using a custom protocol you must provide a serializer");
      }

      final StandardProtocol standardProtocol;
      if (Strings.isNullOrEmpty(customProtocol)) {
        if (serializationProtocol == null) {
          this.serializationProtocol = SerializationProtocol.TCompact;
        }

        switch (serializationProtocol) {
          case TCompact:
            standardProtocol = StandardProtocol.COMPACT;
            break;
          case TJSON:
            standardProtocol = StandardProtocol.JSON;
            break;
          case TBinary:
            standardProtocol = StandardProtocol.BINARY;
            break;
          case TSimpleJSONBase64:
          case TSimpleJSON:
            standardProtocol = StandardProtocol.SIMPLE_JSON;
            break;
          default:
            throw new IllegalArgumentException(
                "Unknown serialization protocol " + serializationProtocol);
        }

        if (value instanceof ThriftSerializable) {
          serializer =
              generateSerializer(((ThriftSerializable) value)::write0, serializationProtocol);
        } else if (value instanceof Writer) {
          serializer = generateSerializer((Writer) value, serializationProtocol);
        } else if (serializer == null) {
          throw new IllegalArgumentException(
              "You must set a serializer if the value is not of ThriftSerializable or Writer");
        }
      } else {
        standardProtocol = StandardProtocol.CUSTOM;
      }

      return new UnserializedLazyAny(
          value,
          customProtocol,
          standardProtocol,
          type,
          serializer,
          useHashPrefix,
          useUri,
          numberOfBytes);
    }
  }

  private static Function<Object, ByteBuf> generateSerializer(
      Writer w, SerializationProtocol serializationProtocol) {
    return o -> {
      ByteBuf dest = RpcResources.getUnpooledByteBufAllocator().buffer(1024, 1 << 24);
      ByteBufTProtocol protocol = SerializerUtil.toByteBufProtocol(serializationProtocol, dest);
      w.write(protocol);

      return dest;
    };
  }
}
