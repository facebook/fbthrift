/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
package com.facebook.thrift.annotation.cpp_deprecated;

import java.util.List;
import java.util.ArrayList;
import java.util.Map;
import java.util.HashMap;
import java.util.Set;
import java.util.HashSet;
import java.util.Collections;
import java.util.BitSet;
import java.util.Arrays;
import com.facebook.thrift.*;
import com.facebook.thrift.annotations.*;
import com.facebook.thrift.async.*;
import com.facebook.thrift.meta_data.*;
import com.facebook.thrift.server.*;
import com.facebook.thrift.transport.*;
import com.facebook.thrift.protocol.*;

/**
 * Causes uses of the given structured type to be replaced with `CursorSerializationWrapper` to allow use of cursor-based serialization.
 * Must add `cpp_include "thrift/lib/cpp2/protocol/CursorBasedSerializer.h"` to files that use this annotation.
 * See documentation for this class in CursorBasedSerializer.h
 * Can only be applied to top-level structs (used as return type or sole argument to an RPC or serialized directly), not to types used as struct fields or container elements.
 */
@SuppressWarnings({ "unused", "serial" })
public class UseCursorSerialization implements TBase, java.io.Serializable, Cloneable {
  private static final TStruct STRUCT_DESC = new TStruct("UseCursorSerialization");


  public UseCursorSerialization() {
  }

  /**
   * Performs a deep copy on <i>other</i>.
   */
  public UseCursorSerialization(UseCursorSerialization other) {
  }

  public UseCursorSerialization deepCopy() {
    return new UseCursorSerialization(this);
  }

  @Override
  public boolean equals(Object _that) {
    if (_that == null)
      return false;
    if (this == _that)
      return true;
    if (!(_that instanceof UseCursorSerialization))
      return false;
    UseCursorSerialization that = (UseCursorSerialization)_that;

    return true;
  }

  @Override
  public int hashCode() {
    return Arrays.deepHashCode(new Object[] {});
  }

  // This is required to satisfy the TBase interface, but can't be implemented on immutable struture.
  public void read(TProtocol iprot) throws TException {
    throw new TException("unimplemented in android immutable structure");
  }

  public static UseCursorSerialization deserialize(TProtocol iprot) throws TException {
    TField __field;
    iprot.readStructBegin();
    while (true)
    {
      __field = iprot.readFieldBegin();
      if (__field.type == TType.STOP) {
        break;
      }
      switch (__field.id)
      {
        default:
          TProtocolUtil.skip(iprot, __field.type);
          break;
      }
      iprot.readFieldEnd();
    }
    iprot.readStructEnd();

    UseCursorSerialization _that;
    _that = new UseCursorSerialization(
    );
    _that.validate();
    return _that;
  }

  public void write(TProtocol oprot) throws TException {
    validate();

    oprot.writeStructBegin(STRUCT_DESC);
    oprot.writeFieldStop();
    oprot.writeStructEnd();
  }

  @Override
  public String toString() {
    return toString(1, true);
  }

  @Override
  public String toString(int indent, boolean prettyPrint) {
    return TBaseHelper.toStringHelper(this, indent, prettyPrint);
  }

  public void validate() throws TException {
    // check for required fields
  }

}
