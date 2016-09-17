/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

package com.facebook.thrift;

import java.io.Serializable;

import com.facebook.thrift.protocol.TProtocol;

/**
 * Generic base interface for generated Thrift objects.
 *
 */
public interface TBase extends Serializable {

  /**
   * Reads the TObject from the given input protocol.
   *
   * @param iprot Input protocol
   */
  public void read(TProtocol iprot) throws TException;

  /**
   * Writes the objects out to the protocol
   *
   * @param oprot Output protocol
   */
  public void write(TProtocol oprot) throws TException;

  /**
   * Check if a field is currently set or unset.
   *
   * @param fieldId The field's id tag as found in the IDL.
   */
  public boolean isSet(int fieldId);

  /**
   * Get a field's value by id. Primitive types will be wrapped in the
   * appropriate "boxed" types.
   *
   * @param fieldId The field's id tag as found in the IDL.
   */
  public Object getFieldValue(int fieldId);

  /**
   * Set a field's value by id. Primitive types must be "boxed" in the
   * appropriate object wrapper type.
   *
   * @param fieldId The field's id tag as found in the IDL.
   */
  public void setFieldValue(int fieldId, Object value);

  /**
   * Returns a copy of `this`. The type of the returned object should
   * be the same as the type of this; that is,
   * <code>x.getClass() == x.deepCopy().getClass()</code> should be true
   * for any TBase.
   */
  public TBase deepCopy();

  /**
   *  Creates an indented String representation for
   *  pretty printing
   *
   *  @param indent The level of indentation desired
   *  @param prettyPrint Set pretty printing on/off
   */

  public String toString(int indent, boolean prettyPrint);

  /**
   * Returns a string representation of an instance
   *
   * @param prettyPrint Set pretty printing on/off
   */
  public String toString(boolean prettyPrint);

}
