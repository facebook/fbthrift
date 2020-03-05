/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
package test.fixtures.basicannotations;

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
import com.facebook.thrift.async.*;
import com.facebook.thrift.meta_data.*;
import com.facebook.thrift.server.*;
import com.facebook.thrift.transport.*;
import com.facebook.thrift.protocol.*;

@SuppressWarnings({ "unused", "serial" })
public class SecretStruct implements TBase, java.io.Serializable, Cloneable, Comparable<SecretStruct> {
  private static final TStruct STRUCT_DESC = new TStruct("SecretStruct");
  private static final TField ID_FIELD_DESC = new TField("id", TType.I64, (short)1);
  private static final TField PASSWORD_FIELD_DESC = new TField("password", TType.STRING, (short)2, new HashMap<String, Object>() {{ put("sensitive", true); }});

  public long id;
  public String password;
  public static final int ID = 1;
  public static final int PASSWORD = 2;

  // isset id assignments
  private static final int __ID_ISSET_ID = 0;
  private BitSet __isset_bit_vector = new BitSet(1);

  public static final Map<Integer, FieldMetaData> metaDataMap;

  static {
    Map<Integer, FieldMetaData> tmpMetaDataMap = new HashMap<Integer, FieldMetaData>();
    tmpMetaDataMap.put(ID, new FieldMetaData("id", TFieldRequirementType.DEFAULT, 
        new FieldValueMetaData(TType.I64)));
    tmpMetaDataMap.put(PASSWORD, new FieldMetaData("password", TFieldRequirementType.DEFAULT, 
        new FieldValueMetaData(TType.STRING)));
    metaDataMap = Collections.unmodifiableMap(tmpMetaDataMap);
  }

  static {
    FieldMetaData.addStructMetaDataMap(SecretStruct.class, metaDataMap);
  }

  public SecretStruct() {
  }

  public SecretStruct(
    long id,
    String password)
  {
    this();
    this.id = id;
    setIdIsSet(true);
    this.password = password;
  }

  public static class Builder {
    private long id;
    private String password;

    BitSet __optional_isset = new BitSet(1);

    public Builder() {
    }

    public Builder setId(final long id) {
      this.id = id;
      __optional_isset.set(__ID_ISSET_ID, true);
      return this;
    }

    public Builder setPassword(final String password) {
      this.password = password;
      return this;
    }

    public SecretStruct build() {
      SecretStruct result = new SecretStruct();
      if (__optional_isset.get(__ID_ISSET_ID)) {
        result.setId(this.id);
      }
      result.setPassword(this.password);
      return result;
    }
  }

  public static Builder builder() {
    return new Builder();
  }

  /**
   * Performs a deep copy on <i>other</i>.
   */
  public SecretStruct(SecretStruct other) {
    __isset_bit_vector.clear();
    __isset_bit_vector.or(other.__isset_bit_vector);
    this.id = TBaseHelper.deepCopy(other.id);
    if (other.isSetPassword()) {
      this.password = TBaseHelper.deepCopy(other.password);
    }
  }

  public SecretStruct deepCopy() {
    return new SecretStruct(this);
  }

  public long getId() {
    return this.id;
  }

  public SecretStruct setId(long id) {
    this.id = id;
    setIdIsSet(true);
    return this;
  }

  public void unsetId() {
    __isset_bit_vector.clear(__ID_ISSET_ID);
  }

  // Returns true if field id is set (has been assigned a value) and false otherwise
  public boolean isSetId() {
    return __isset_bit_vector.get(__ID_ISSET_ID);
  }

  public void setIdIsSet(boolean __value) {
    __isset_bit_vector.set(__ID_ISSET_ID, __value);
  }

  public String getPassword() {
    return this.password;
  }

  public SecretStruct setPassword(String password) {
    this.password = password;
    return this;
  }

  public void unsetPassword() {
    this.password = null;
  }

  // Returns true if field password is set (has been assigned a value) and false otherwise
  public boolean isSetPassword() {
    return this.password != null;
  }

  public void setPasswordIsSet(boolean __value) {
    if (!__value) {
      this.password = null;
    }
  }

  public void setFieldValue(int fieldID, Object __value) {
    switch (fieldID) {
    case ID:
      if (__value == null) {
        unsetId();
      } else {
        setId((Long)__value);
      }
      break;

    case PASSWORD:
      if (__value == null) {
        unsetPassword();
      } else {
        setPassword((String)__value);
      }
      break;

    default:
      throw new IllegalArgumentException("Field " + fieldID + " doesn't exist!");
    }
  }

  public Object getFieldValue(int fieldID) {
    switch (fieldID) {
    case ID:
      return new Long(getId());

    case PASSWORD:
      return getPassword();

    default:
      throw new IllegalArgumentException("Field " + fieldID + " doesn't exist!");
    }
  }

  @Override
  public boolean equals(Object that) {
    if (that == null)
      return false;
    if (that instanceof SecretStruct)
      return this.equals((SecretStruct)that);
    return false;
  }

  public boolean equals(SecretStruct that) {
    if (that == null)
      return false;
    if (this == that)
      return true;

    boolean this_present_id = true;
    boolean that_present_id = true;
    if (this_present_id || that_present_id) {
      if (!(this_present_id && that_present_id))
        return false;
      if (!TBaseHelper.equalsNobinary(this.id, that.id))
        return false;
    }

    boolean this_present_password = true && this.isSetPassword();
    boolean that_present_password = true && that.isSetPassword();
    if (this_present_password || that_present_password) {
      if (!(this_present_password && that_present_password))
        return false;
      if (!TBaseHelper.equalsNobinary(this.password, that.password))
        return false;
    }

    return true;
  }

  @Override
  public int hashCode() {
    return Arrays.deepHashCode(new Object[] {id, password});
  }

  @Override
  public int compareTo(SecretStruct other) {
    if (other == null) {
      // See java.lang.Comparable docs
      throw new NullPointerException();
    }

    if (other == this) {
      return 0;
    }
    int lastComparison = 0;

    lastComparison = Boolean.valueOf(isSetId()).compareTo(other.isSetId());
    if (lastComparison != 0) {
      return lastComparison;
    }
    lastComparison = TBaseHelper.compareTo(id, other.id);
    if (lastComparison != 0) {
      return lastComparison;
    }
    lastComparison = Boolean.valueOf(isSetPassword()).compareTo(other.isSetPassword());
    if (lastComparison != 0) {
      return lastComparison;
    }
    lastComparison = TBaseHelper.compareTo(password, other.password);
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
        case ID:
          if (__field.type == TType.I64) {
            this.id = iprot.readI64();
            setIdIsSet(true);
          } else { 
            TProtocolUtil.skip(iprot, __field.type);
          }
          break;
        case PASSWORD:
          if (__field.type == TType.STRING) {
            this.password = iprot.readString();
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
    oprot.writeFieldBegin(ID_FIELD_DESC);
    oprot.writeI64(this.id);
    oprot.writeFieldEnd();
    if (this.password != null) {
      oprot.writeFieldBegin(PASSWORD_FIELD_DESC);
      oprot.writeString(this.password);
      oprot.writeFieldEnd();
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
    StringBuilder sb = new StringBuilder("SecretStruct");
    sb.append(space);
    sb.append("(");
    sb.append(newLine);
    boolean first = true;

    sb.append(indentStr);
    sb.append("id");
    sb.append(space);
    sb.append(":").append(space);
    sb.append(TBaseHelper.toString(this.getId(), indent + 1, prettyPrint));
    first = false;
    if (!first) sb.append("," + newLine);
    sb.append(indentStr);
    sb.append("password");
    sb.append(space);
    sb.append(":").append(space);
    sb.append("<SENSITIVE FIELD>");
    first = false;
    sb.append(newLine + TBaseHelper.reduceIndent(indentStr));
    sb.append(")");
    return sb.toString();
  }

  public void validate() throws TException {
    // check for required fields
  }

}

