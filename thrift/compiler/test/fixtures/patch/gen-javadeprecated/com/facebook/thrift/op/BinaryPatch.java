/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
package com.facebook.thrift.op;

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
 * A patch for a binary value.
 */
@SuppressWarnings({ "unused", "serial" })
public class BinaryPatch implements TBase, java.io.Serializable, Cloneable, Comparable<BinaryPatch> {
  private static final TStruct STRUCT_DESC = new TStruct("BinaryPatch");
  private static final TField ASSIGN_FIELD_DESC = new TField("assign", TType.STRING, (short)1);

  /**
   * Assign to a given value.
   * 
   * If set, all other patch operations are ignored.
   */
  public byte[] assign;
  public static final int ASSIGN = 1;

  // isset id assignments

  public static final Map<Integer, FieldMetaData> metaDataMap;

  static {
    Map<Integer, FieldMetaData> tmpMetaDataMap = new HashMap<Integer, FieldMetaData>();
    tmpMetaDataMap.put(ASSIGN, new FieldMetaData("assign", TFieldRequirementType.OPTIONAL, 
        new FieldValueMetaData(TType.STRING)));
    metaDataMap = Collections.unmodifiableMap(tmpMetaDataMap);
  }

  static {
    FieldMetaData.addStructMetaDataMap(BinaryPatch.class, metaDataMap);
  }

  public BinaryPatch() {
  }

  public BinaryPatch(
      byte[] assign) {
    this();
    this.assign = assign;
  }

  public static class Builder {
    private byte[] assign;

    public Builder() {
    }

    public Builder setAssign(final byte[] assign) {
      this.assign = assign;
      return this;
    }

    public BinaryPatch build() {
      BinaryPatch result = new BinaryPatch();
      result.setAssign(this.assign);
      return result;
    }
  }

  public static Builder builder() {
    return new Builder();
  }

  /**
   * Performs a deep copy on <i>other</i>.
   */
  public BinaryPatch(BinaryPatch other) {
    if (other.isSetAssign()) {
      this.assign = TBaseHelper.deepCopy(other.assign);
    }
  }

  public BinaryPatch deepCopy() {
    return new BinaryPatch(this);
  }

  /**
   * Assign to a given value.
   * 
   * If set, all other patch operations are ignored.
   */
  public byte[] getAssign() {
    return this.assign;
  }

  /**
   * Assign to a given value.
   * 
   * If set, all other patch operations are ignored.
   */
  public BinaryPatch setAssign(byte[] assign) {
    this.assign = assign;
    return this;
  }

  public void unsetAssign() {
    this.assign = null;
  }

  // Returns true if field assign is set (has been assigned a value) and false otherwise
  public boolean isSetAssign() {
    return this.assign != null;
  }

  public void setAssignIsSet(boolean __value) {
    if (!__value) {
      this.assign = null;
    }
  }

  public void setFieldValue(int fieldID, Object __value) {
    switch (fieldID) {
    case ASSIGN:
      if (__value == null) {
        unsetAssign();
      } else {
        setAssign((byte[])__value);
      }
      break;

    default:
      throw new IllegalArgumentException("Field " + fieldID + " doesn't exist!");
    }
  }

  public Object getFieldValue(int fieldID) {
    switch (fieldID) {
    case ASSIGN:
      return getAssign();

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
    if (!(_that instanceof BinaryPatch))
      return false;
    BinaryPatch that = (BinaryPatch)_that;

    if (!TBaseHelper.equalsSlow(this.isSetAssign(), that.isSetAssign(), this.assign, that.assign)) { return false; }

    return true;
  }

  @Override
  public int hashCode() {
    return Arrays.deepHashCode(new Object[] {assign});
  }

  @Override
  public int compareTo(BinaryPatch other) {
    if (other == null) {
      // See java.lang.Comparable docs
      throw new NullPointerException();
    }

    if (other == this) {
      return 0;
    }
    int lastComparison = 0;

    lastComparison = Boolean.valueOf(isSetAssign()).compareTo(other.isSetAssign());
    if (lastComparison != 0) {
      return lastComparison;
    }
    lastComparison = TBaseHelper.compareTo(assign, other.assign);
    if (lastComparison != 0) { 
      return lastComparison;
    }
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
        case ASSIGN:
          if (__field.type == TType.STRING) {
            this.assign = iprot.readBinary();
          } else {
            TProtocolUtil.skip(iprot, __field.type);
          }
          break;
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
    if (this.assign != null) {
      if (isSetAssign()) {
        oprot.writeFieldBegin(ASSIGN_FIELD_DESC);
        oprot.writeBinary(this.assign);
        oprot.writeFieldEnd();
      }
    }
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
    StringBuilder sb = new StringBuilder("BinaryPatch");
    sb.append(space);
    sb.append("(");
    sb.append(newLine);
    boolean first = true;

    if (isSetAssign())
    {
      sb.append(indentStr);
      sb.append("assign");
      sb.append(space);
      sb.append(":").append(space);
      if (this.getAssign() == null) {
        sb.append("null");
      } else {
          int __assign_size = Math.min(this.getAssign().length, 128);
          for (int i = 0; i < __assign_size; i++) {
            if (i != 0) sb.append(" ");
            sb.append(Integer.toHexString(this.getAssign()[i]).length() > 1 ? Integer.toHexString(this.getAssign()[i]).substring(Integer.toHexString(this.getAssign()[i]).length() - 2).toUpperCase() : "0" + Integer.toHexString(this.getAssign()[i]).toUpperCase());
          }
          if (this.getAssign().length > 128) sb.append(" ...");
      }
      first = false;
    }
    sb.append(newLine + TBaseHelper.reduceIndent(indentStr));
    sb.append(")");
    return sb.toString();
  }

  public void validate() throws TException {
    // check for required fields
  }

}

