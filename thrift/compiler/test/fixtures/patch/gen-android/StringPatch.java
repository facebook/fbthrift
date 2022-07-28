/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
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
 * A patch for a string value.
 */
@SuppressWarnings({ "unused", "serial" })
public class StringPatch implements TBase, java.io.Serializable, Cloneable {
  private static final TStruct STRUCT_DESC = new TStruct("StringPatch");
  private static final TField ASSIGN_FIELD_DESC = new TField("assign", TType.STRING, (short)1);
  private static final TField CLEAR_FIELD_DESC = new TField("clear", TType.BOOL, (short)2);
  private static final TField APPEND_FIELD_DESC = new TField("append", TType.STRING, (short)9);
  private static final TField PREPEND_FIELD_DESC = new TField("prepend", TType.STRING, (short)10);

  /**
   * Assign to a given value.
   * 
   * If set, all other patch operations are ignored.
   */
  public final String assign;
  /**
   * Clear a given string.
   */
  public final Boolean clear;
  /**
   * Append to a given value.
   */
  public final String append;
  /**
   * Prepend to a given value.
   */
  public final String prepend;
  public static final int ASSIGN = 1;
  public static final int CLEAR = 2;
  public static final int APPEND = 9;
  public static final int PREPEND = 10;

  public StringPatch(
      String assign,
      Boolean clear,
      String append,
      String prepend) {
    this.assign = assign;
    this.clear = clear;
    this.append = append;
    this.prepend = prepend;
  }

  /**
   * Performs a deep copy on <i>other</i>.
   */
  public StringPatch(StringPatch other) {
    if (other.isSetAssign()) {
      this.assign = TBaseHelper.deepCopy(other.assign);
    } else {
      this.assign = null;
    }
    if (other.isSetClear()) {
      this.clear = TBaseHelper.deepCopy(other.clear);
    } else {
      this.clear = null;
    }
    if (other.isSetAppend()) {
      this.append = TBaseHelper.deepCopy(other.append);
    } else {
      this.append = null;
    }
    if (other.isSetPrepend()) {
      this.prepend = TBaseHelper.deepCopy(other.prepend);
    } else {
      this.prepend = null;
    }
  }

  public StringPatch deepCopy() {
    return new StringPatch(this);
  }

  /**
   * Assign to a given value.
   * 
   * If set, all other patch operations are ignored.
   */
  public String getAssign() {
    return this.assign;
  }

  // Returns true if field assign is set (has been assigned a value) and false otherwise
  public boolean isSetAssign() {
    return this.assign != null;
  }

  /**
   * Clear a given string.
   */
  public Boolean isClear() {
    return this.clear;
  }

  // Returns true if field clear is set (has been assigned a value) and false otherwise
  public boolean isSetClear() {
    return this.clear != null;
  }

  /**
   * Append to a given value.
   */
  public String getAppend() {
    return this.append;
  }

  // Returns true if field append is set (has been assigned a value) and false otherwise
  public boolean isSetAppend() {
    return this.append != null;
  }

  /**
   * Prepend to a given value.
   */
  public String getPrepend() {
    return this.prepend;
  }

  // Returns true if field prepend is set (has been assigned a value) and false otherwise
  public boolean isSetPrepend() {
    return this.prepend != null;
  }

  @Override
  public boolean equals(Object _that) {
    if (_that == null)
      return false;
    if (this == _that)
      return true;
    if (!(_that instanceof StringPatch))
      return false;
    StringPatch that = (StringPatch)_that;

    if (!TBaseHelper.equalsNobinary(this.isSetAssign(), that.isSetAssign(), this.assign, that.assign)) { return false; }

    if (!TBaseHelper.equalsNobinary(this.isSetClear(), that.isSetClear(), this.clear, that.clear)) { return false; }

    if (!TBaseHelper.equalsNobinary(this.isSetAppend(), that.isSetAppend(), this.append, that.append)) { return false; }

    if (!TBaseHelper.equalsNobinary(this.isSetPrepend(), that.isSetPrepend(), this.prepend, that.prepend)) { return false; }

    return true;
  }

  @Override
  public int hashCode() {
    return Arrays.deepHashCode(new Object[] {assign, clear, append, prepend});
  }

  // This is required to satisfy the TBase interface, but can't be implemented on immutable struture.
  public void read(TProtocol iprot) throws TException {
    throw new TException("unimplemented in android immutable structure");
  }

  public static StringPatch deserialize(TProtocol iprot) throws TException {
    String tmp_assign = null;
    Boolean tmp_clear = null;
    String tmp_append = null;
    String tmp_prepend = null;
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
        case ASSIGN:
          if (__field.type == TType.STRING) {
            tmp_assign = iprot.readString();
          } else {
            TProtocolUtil.skip(iprot, __field.type);
          }
          break;
        case CLEAR:
          if (__field.type == TType.BOOL) {
            tmp_clear = iprot.readBool();
          } else {
            TProtocolUtil.skip(iprot, __field.type);
          }
          break;
        case APPEND:
          if (__field.type == TType.STRING) {
            tmp_append = iprot.readString();
          } else {
            TProtocolUtil.skip(iprot, __field.type);
          }
          break;
        case PREPEND:
          if (__field.type == TType.STRING) {
            tmp_prepend = iprot.readString();
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

    StringPatch _that;
    _that = new StringPatch(
      tmp_assign
      ,tmp_clear
      ,tmp_append
      ,tmp_prepend
    );
    _that.validate();
    return _that;
  }

  public void write(TProtocol oprot) throws TException {
    validate();

    oprot.writeStructBegin(STRUCT_DESC);
    if (this.assign != null) {
      if (isSetAssign()) {
        oprot.writeFieldBegin(ASSIGN_FIELD_DESC);
        oprot.writeString(this.assign);
        oprot.writeFieldEnd();
      }
    }
    if (this.clear != null) {
      oprot.writeFieldBegin(CLEAR_FIELD_DESC);
      oprot.writeBool(this.clear);
      oprot.writeFieldEnd();
    }
    if (this.append != null) {
      oprot.writeFieldBegin(APPEND_FIELD_DESC);
      oprot.writeString(this.append);
      oprot.writeFieldEnd();
    }
    if (this.prepend != null) {
      oprot.writeFieldBegin(PREPEND_FIELD_DESC);
      oprot.writeString(this.prepend);
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
    return TBaseHelper.toStringHelper(this, indent, prettyPrint);
  }

  public void validate() throws TException {
    // check for required fields
  }

}

