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
 * Allows the field to be annotated @cpp.Ref (or cpp[2].ref[_type]) even if it
 * is not optional (or in a union, which is effectively optional).
 * 
 * This annotation is provided for a limited time, to exempt pre-existing fields
 * while rolling out a stricter enforcement of the condition above.
 * 
 * Reminder: reference fields should be optional because the corresponding smart
 * pointers (std::unique_ptr, std::shared_ptr) can always be reset or set to
 * nullptr by the clients. If the field is not optional, this leads to a
 * confusing (or non-sensical) situation, wherein a field that should always
 * have a value has nullptr instead.
 */
@SuppressWarnings({ "unused", "serial" })
public class AllowLegacyNonOptionalRef implements TBase, java.io.Serializable, Cloneable, Comparable<AllowLegacyNonOptionalRef> {
  private static final TStruct STRUCT_DESC = new TStruct("AllowLegacyNonOptionalRef");

  public static final Map<Integer, FieldMetaData> metaDataMap;

  static {
    Map<Integer, FieldMetaData> tmpMetaDataMap = new HashMap<Integer, FieldMetaData>();
    metaDataMap = Collections.unmodifiableMap(tmpMetaDataMap);
  }

  static {
    FieldMetaData.addStructMetaDataMap(AllowLegacyNonOptionalRef.class, metaDataMap);
  }

  public AllowLegacyNonOptionalRef() {
  }

  public static class Builder {

    public Builder() {
    }

    public AllowLegacyNonOptionalRef build() {
      AllowLegacyNonOptionalRef result = new AllowLegacyNonOptionalRef();
      return result;
    }
  }

  public static Builder builder() {
    return new Builder();
  }

  /**
   * Performs a deep copy on <i>other</i>.
   */
  public AllowLegacyNonOptionalRef(AllowLegacyNonOptionalRef other) {
  }

  public AllowLegacyNonOptionalRef deepCopy() {
    return new AllowLegacyNonOptionalRef(this);
  }

  public void setFieldValue(int fieldID, Object __value) {
    switch (fieldID) {
    default:
      throw new IllegalArgumentException("Field " + fieldID + " doesn't exist!");
    }
  }

  public Object getFieldValue(int fieldID) {
    switch (fieldID) {
    default:
      throw new IllegalArgumentException("Field " + fieldID + " doesn't exist!");
    }
  }

  @Override
  public boolean equals(Object _that) {
    if (_that == null)
      return false;
    if (this == _that)
      return true;
    if (!(_that instanceof AllowLegacyNonOptionalRef))
      return false;
    AllowLegacyNonOptionalRef that = (AllowLegacyNonOptionalRef)_that;

    return true;
  }

  @Override
  public int hashCode() {
    return Arrays.deepHashCode(new Object[] {});
  }

  @Override
  public int compareTo(AllowLegacyNonOptionalRef other) {
    if (other == null) {
      // See java.lang.Comparable docs
      throw new NullPointerException();
    }

    if (other == this) {
      return 0;
    }
    int lastComparison = 0;

    return 0;
  }

  public void read(TProtocol iprot) throws TException {
    TField __field;
    iprot.readStructBegin(metaDataMap);
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


    // check for required fields of primitive type, which can't be checked in the validate method
    validate();
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
    String indentStr = prettyPrint ? TBaseHelper.getIndentedString(indent) : "";
    String newLine = prettyPrint ? "\n" : "";
    String space = prettyPrint ? " " : "";
    StringBuilder sb = new StringBuilder("AllowLegacyNonOptionalRef");
    sb.append(space);
    sb.append("(");
    sb.append(newLine);
    boolean first = true;

    sb.append(newLine + TBaseHelper.reduceIndent(indentStr));
    sb.append(")");
    return sb.toString();
  }

  public void validate() throws TException {
    // check for required fields
  }

}

