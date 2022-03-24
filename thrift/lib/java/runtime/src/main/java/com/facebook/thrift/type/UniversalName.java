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

import static com.facebook.thrift.type.UniversalHashAlgorithm.SHA256;

import io.netty.buffer.ByteBuf;
import io.netty.buffer.ByteBufUtil;
import io.netty.buffer.Unpooled;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.regex.Pattern;

public class UniversalName {

  private static final String THRIFT_SCHEME = "fbthrift://";

  private static final String DOMAIN_NAME_PATTERN =
      "^((?!-)[A-Za-z0-9-]{1,63}(?<!-)\\.)+[A-Za-z]{2,6}/";
  private static final String PACKAGE_NAME_PATTERN = "(([a-zA-Z0-9_-]{1,})/)+";
  private static final String TYPE_PATTERN = "[a-zA-Z0-9_-]{1,}";
  private static final String UNIVERSAL_NAME_PATTERN =
      DOMAIN_NAME_PATTERN + PACKAGE_NAME_PATTERN + TYPE_PATTERN;

  private static Pattern namePattern;

  static {
    namePattern = Pattern.compile(UNIVERSAL_NAME_PATTERN);
  }

  private final UniversalHashAlgorithm algorithm;
  private final String uri;
  private ByteBuf hash;

  public UniversalName(String uri, UniversalHashAlgorithm algorithm)
      throws InvalidUniversalNameURIException {
    if (!namePattern.matcher(uri).matches()) {
      throw new InvalidUniversalNameURIException(uri);
    }
    this.uri = uri;
    this.algorithm = algorithm;
    generateHash();
  }

  public UniversalName(String uri) throws InvalidUniversalNameURIException {
    this(uri, SHA256);
  }

  private void generateHash() {
    try {
      switch (algorithm) {
        case SHA256:
          {
            MessageDigest digest = MessageDigest.getInstance("SHA-256");
            hash =
                Unpooled.wrappedBuffer(
                    digest.digest((THRIFT_SCHEME + this.uri).getBytes(StandardCharsets.UTF_8)));
            break;
          }
        default:
          throw new RuntimeException("Algorithm not supported, " + algorithm);
      }
    } catch (NoSuchAlgorithmException e) {
      throw new RuntimeException(e);
    }
  }

  public UniversalHashAlgorithm getAlgorithm() {
    return this.algorithm;
  }

  public String getHash() {
    return getHashPrefix(this.algorithm.getDefaultHashBytes());
  }

  public ByteBuf getHashBytes() {
    return getHashPrefixBytes(this.algorithm.getDefaultHashBytes());
  }

  public String getHashPrefix(int size) {
    return ByteBufUtil.hexDump(
        this.hash,
        0,
        Math.min(
            Math.max(size, this.algorithm.getMinHashBytes()),
            this.algorithm.getDefaultHashBytes()));
  }

  public ByteBuf getHashPrefixBytes(int size) {
    return this.hash.copy(
        0,
        Math.min(
            Math.max(size, this.algorithm.getMinHashBytes()),
            this.algorithm.getDefaultHashBytes()));
  }

  public String getUri() {
    return this.uri;
  }

  public boolean preferHash() {
    return this.algorithm.getDefaultHashBytes() < this.uri.length();
  }

  public boolean preferHash(int hashSize) {
    return Math.min(
            this.algorithm.getDefaultHashBytes(),
            Math.max(hashSize, this.algorithm.getMinHashBytes()))
        < this.uri.length();
  }

  @Override
  public String toString() {
    return "UniversalName{"
        + "algorithm="
        + algorithm
        + ", uri='"
        + uri
        + '\''
        + ", hash="
        + getHash()
        + '}';
  }
}
