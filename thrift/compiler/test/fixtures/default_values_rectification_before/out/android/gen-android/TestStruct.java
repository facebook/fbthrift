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

@SuppressWarnings({ "unused", "serial" })
public class TestStruct implements TBase, java.io.Serializable, Cloneable {
  private static final TStruct STRUCT_DESC = new TStruct("TestStruct");
  private static final TField UNQUALIFIED_INT_FIELD_FIELD_DESC = new TField("unqualified_int_field", TType.I32, (short)1);
  private static final TField UNQUALIFIED_BOOL_FIELD_FIELD_DESC = new TField("unqualified_bool_field", TType.BOOL, (short)2);
  private static final TField UNQUALIFIED_LIST_FIELD_FIELD_DESC = new TField("unqualified_list_field", TType.LIST, (short)3);
  private static final TField UNQUALIFIED_STRUCT_FIELD_FIELD_DESC = new TField("unqualified_struct_field", TType.STRUCT, (short)4);
  private static final TField OPTIONAL_INT_FIELD_FIELD_DESC = new TField("optional_int_field", TType.I32, (short)5);
  private static final TField OPTIONAL_BOOL_FIELD_FIELD_DESC = new TField("optional_bool_field", TType.BOOL, (short)6);
  private static final TField OPTIONAL_LIST_FIELD_FIELD_DESC = new TField("optional_list_field", TType.LIST, (short)7);
  private static final TField OPTIONAL_STRUCT_FIELD_FIELD_DESC = new TField("optional_struct_field", TType.STRUCT, (short)8);

  public final Integer unqualified_int_field;
  public final Boolean unqualified_bool_field;
  public final List<Integer> unqualified_list_field;
  public final EmptyStruct unqualified_struct_field;
  public final Integer optional_int_field;
  public final Boolean optional_bool_field;
  public final List<Integer> optional_list_field;
  public final EmptyStruct optional_struct_field;
  public static final int UNQUALIFIED_INT_FIELD = 1;
  public static final int UNQUALIFIED_BOOL_FIELD = 2;
  public static final int UNQUALIFIED_LIST_FIELD = 3;
  public static final int UNQUALIFIED_STRUCT_FIELD = 4;
  public static final int OPTIONAL_INT_FIELD = 5;
  public static final int OPTIONAL_BOOL_FIELD = 6;
  public static final int OPTIONAL_LIST_FIELD = 7;
  public static final int OPTIONAL_STRUCT_FIELD = 8;

  public TestStruct(
      Integer unqualified_int_field,
      Boolean unqualified_bool_field,
      List<Integer> unqualified_list_field,
      EmptyStruct unqualified_struct_field,
      Integer optional_int_field,
      Boolean optional_bool_field,
      List<Integer> optional_list_field,
      EmptyStruct optional_struct_field) {
    this.unqualified_int_field = unqualified_int_field;
    this.unqualified_bool_field = unqualified_bool_field;
    this.unqualified_list_field = unqualified_list_field;
    this.unqualified_struct_field = unqualified_struct_field;
    this.optional_int_field = optional_int_field;
    this.optional_bool_field = optional_bool_field;
    this.optional_list_field = optional_list_field;
    this.optional_struct_field = optional_struct_field;
  }

  /**
   * Performs a deep copy on <i>other</i>.
   */
  public TestStruct(TestStruct other) {
    if (other.isSetUnqualified_int_field()) {
      this.unqualified_int_field = TBaseHelper.deepCopy(other.unqualified_int_field);
    } else {
      this.unqualified_int_field = null;
    }
    if (other.isSetUnqualified_bool_field()) {
      this.unqualified_bool_field = TBaseHelper.deepCopy(other.unqualified_bool_field);
    } else {
      this.unqualified_bool_field = null;
    }
    if (other.isSetUnqualified_list_field()) {
      this.unqualified_list_field = TBaseHelper.deepCopy(other.unqualified_list_field);
    } else {
      this.unqualified_list_field = null;
    }
    if (other.isSetUnqualified_struct_field()) {
      this.unqualified_struct_field = TBaseHelper.deepCopy(other.unqualified_struct_field);
    } else {
      this.unqualified_struct_field = null;
    }
    if (other.isSetOptional_int_field()) {
      this.optional_int_field = TBaseHelper.deepCopy(other.optional_int_field);
    } else {
      this.optional_int_field = null;
    }
    if (other.isSetOptional_bool_field()) {
      this.optional_bool_field = TBaseHelper.deepCopy(other.optional_bool_field);
    } else {
      this.optional_bool_field = null;
    }
    if (other.isSetOptional_list_field()) {
      this.optional_list_field = TBaseHelper.deepCopy(other.optional_list_field);
    } else {
      this.optional_list_field = null;
    }
    if (other.isSetOptional_struct_field()) {
      this.optional_struct_field = TBaseHelper.deepCopy(other.optional_struct_field);
    } else {
      this.optional_struct_field = null;
    }
  }

  public TestStruct deepCopy() {
    return new TestStruct(this);
  }

  public Integer getUnqualified_int_field() {
    return this.unqualified_int_field;
  }

  // Returns true if field unqualified_int_field is set (has been assigned a value) and false otherwise
  public boolean isSetUnqualified_int_field() {
    return this.unqualified_int_field != null;
  }

  public Boolean isUnqualified_bool_field() {
    return this.unqualified_bool_field;
  }

  // Returns true if field unqualified_bool_field is set (has been assigned a value) and false otherwise
  public boolean isSetUnqualified_bool_field() {
    return this.unqualified_bool_field != null;
  }

  public List<Integer> getUnqualified_list_field() {
    return this.unqualified_list_field;
  }

  // Returns true if field unqualified_list_field is set (has been assigned a value) and false otherwise
  public boolean isSetUnqualified_list_field() {
    return this.unqualified_list_field != null;
  }

  public EmptyStruct getUnqualified_struct_field() {
    return this.unqualified_struct_field;
  }

  // Returns true if field unqualified_struct_field is set (has been assigned a value) and false otherwise
  public boolean isSetUnqualified_struct_field() {
    return this.unqualified_struct_field != null;
  }

  public Integer getOptional_int_field() {
    return this.optional_int_field;
  }

  // Returns true if field optional_int_field is set (has been assigned a value) and false otherwise
  public boolean isSetOptional_int_field() {
    return this.optional_int_field != null;
  }

  public Boolean isOptional_bool_field() {
    return this.optional_bool_field;
  }

  // Returns true if field optional_bool_field is set (has been assigned a value) and false otherwise
  public boolean isSetOptional_bool_field() {
    return this.optional_bool_field != null;
  }

  public List<Integer> getOptional_list_field() {
    return this.optional_list_field;
  }

  // Returns true if field optional_list_field is set (has been assigned a value) and false otherwise
  public boolean isSetOptional_list_field() {
    return this.optional_list_field != null;
  }

  public EmptyStruct getOptional_struct_field() {
    return this.optional_struct_field;
  }

  // Returns true if field optional_struct_field is set (has been assigned a value) and false otherwise
  public boolean isSetOptional_struct_field() {
    return this.optional_struct_field != null;
  }

  @Override
  public boolean equals(Object _that) {
    if (_that == null)
      return false;
    if (this == _that)
      return true;
    if (!(_that instanceof TestStruct))
      return false;
    TestStruct that = (TestStruct)_that;

    if (!TBaseHelper.equalsNobinary(this.isSetUnqualified_int_field(), that.isSetUnqualified_int_field(), this.unqualified_int_field, that.unqualified_int_field)) { return false; }

    if (!TBaseHelper.equalsNobinary(this.isSetUnqualified_bool_field(), that.isSetUnqualified_bool_field(), this.unqualified_bool_field, that.unqualified_bool_field)) { return false; }

    if (!TBaseHelper.equalsNobinary(this.isSetUnqualified_list_field(), that.isSetUnqualified_list_field(), this.unqualified_list_field, that.unqualified_list_field)) { return false; }

    if (!TBaseHelper.equalsNobinary(this.isSetUnqualified_struct_field(), that.isSetUnqualified_struct_field(), this.unqualified_struct_field, that.unqualified_struct_field)) { return false; }

    if (!TBaseHelper.equalsNobinary(this.isSetOptional_int_field(), that.isSetOptional_int_field(), this.optional_int_field, that.optional_int_field)) { return false; }

    if (!TBaseHelper.equalsNobinary(this.isSetOptional_bool_field(), that.isSetOptional_bool_field(), this.optional_bool_field, that.optional_bool_field)) { return false; }

    if (!TBaseHelper.equalsNobinary(this.isSetOptional_list_field(), that.isSetOptional_list_field(), this.optional_list_field, that.optional_list_field)) { return false; }

    if (!TBaseHelper.equalsNobinary(this.isSetOptional_struct_field(), that.isSetOptional_struct_field(), this.optional_struct_field, that.optional_struct_field)) { return false; }

    return true;
  }

  @Override
  public int hashCode() {
    return Arrays.deepHashCode(new Object[] {unqualified_int_field, unqualified_bool_field, unqualified_list_field, unqualified_struct_field, optional_int_field, optional_bool_field, optional_list_field, optional_struct_field});
  }

  // This is required to satisfy the TBase interface, but can't be implemented on immutable struture.
  public void read(TProtocol iprot) throws TException {
    throw new TException("unimplemented in android immutable structure");
  }

  public static TestStruct deserialize(TProtocol iprot) throws TException {
    Integer tmp_unqualified_int_field = null;
    Boolean tmp_unqualified_bool_field = null;
    List<Integer> tmp_unqualified_list_field = null;
    EmptyStruct tmp_unqualified_struct_field = null;
    Integer tmp_optional_int_field = null;
    Boolean tmp_optional_bool_field = null;
    List<Integer> tmp_optional_list_field = null;
    EmptyStruct tmp_optional_struct_field = null;
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
        case UNQUALIFIED_INT_FIELD:
          if (__field.type == TType.I32) {
            tmp_unqualified_int_field = iprot.readI32();
          } else {
            TProtocolUtil.skip(iprot, __field.type);
          }
          break;
        case UNQUALIFIED_BOOL_FIELD:
          if (__field.type == TType.BOOL) {
            tmp_unqualified_bool_field = iprot.readBool();
          } else {
            TProtocolUtil.skip(iprot, __field.type);
          }
          break;
        case UNQUALIFIED_LIST_FIELD:
          if (__field.type == TType.LIST) {
            {
              TList _list0 = iprot.readListBegin();
              tmp_unqualified_list_field = new ArrayList<Integer>(Math.max(0, _list0.size));
              for (int _i1 = 0; 
                   (_list0.size < 0) ? iprot.peekList() : (_i1 < _list0.size); 
                   ++_i1)
              {
                Integer _elem2;
                _elem2 = iprot.readI32();
                tmp_unqualified_list_field.add(_elem2);
              }
              iprot.readListEnd();
            }
          } else {
            TProtocolUtil.skip(iprot, __field.type);
          }
          break;
        case UNQUALIFIED_STRUCT_FIELD:
          if (__field.type == TType.STRUCT) {
            tmp_unqualified_struct_field = EmptyStruct.deserialize(iprot);
          } else {
            TProtocolUtil.skip(iprot, __field.type);
          }
          break;
        case OPTIONAL_INT_FIELD:
          if (__field.type == TType.I32) {
            tmp_optional_int_field = iprot.readI32();
          } else {
            TProtocolUtil.skip(iprot, __field.type);
          }
          break;
        case OPTIONAL_BOOL_FIELD:
          if (__field.type == TType.BOOL) {
            tmp_optional_bool_field = iprot.readBool();
          } else {
            TProtocolUtil.skip(iprot, __field.type);
          }
          break;
        case OPTIONAL_LIST_FIELD:
          if (__field.type == TType.LIST) {
            {
              TList _list3 = iprot.readListBegin();
              tmp_optional_list_field = new ArrayList<Integer>(Math.max(0, _list3.size));
              for (int _i4 = 0; 
                   (_list3.size < 0) ? iprot.peekList() : (_i4 < _list3.size); 
                   ++_i4)
              {
                Integer _elem5;
                _elem5 = iprot.readI32();
                tmp_optional_list_field.add(_elem5);
              }
              iprot.readListEnd();
            }
          } else {
            TProtocolUtil.skip(iprot, __field.type);
          }
          break;
        case OPTIONAL_STRUCT_FIELD:
          if (__field.type == TType.STRUCT) {
            tmp_optional_struct_field = EmptyStruct.deserialize(iprot);
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

    TestStruct _that;
    _that = new TestStruct(
      tmp_unqualified_int_field
      ,tmp_unqualified_bool_field
      ,tmp_unqualified_list_field
      ,tmp_unqualified_struct_field
      ,tmp_optional_int_field
      ,tmp_optional_bool_field
      ,tmp_optional_list_field
      ,tmp_optional_struct_field
    );
    _that.validate();
    return _that;
  }

  public void write(TProtocol oprot) throws TException {
    validate();

    oprot.writeStructBegin(STRUCT_DESC);
    if (this.unqualified_int_field != null) {
      oprot.writeFieldBegin(UNQUALIFIED_INT_FIELD_FIELD_DESC);
      oprot.writeI32(this.unqualified_int_field);
      oprot.writeFieldEnd();
    }
    if (this.unqualified_bool_field != null) {
      oprot.writeFieldBegin(UNQUALIFIED_BOOL_FIELD_FIELD_DESC);
      oprot.writeBool(this.unqualified_bool_field);
      oprot.writeFieldEnd();
    }
    if (this.unqualified_list_field != null) {
      oprot.writeFieldBegin(UNQUALIFIED_LIST_FIELD_FIELD_DESC);
      {
        oprot.writeListBegin(new TList(TType.I32, this.unqualified_list_field.size()));
        for (Integer _iter6 : this.unqualified_list_field)        {
          oprot.writeI32(_iter6);
        }
        oprot.writeListEnd();
      }
      oprot.writeFieldEnd();
    }
    if (this.unqualified_struct_field != null) {
      oprot.writeFieldBegin(UNQUALIFIED_STRUCT_FIELD_FIELD_DESC);
      this.unqualified_struct_field.write(oprot);
      oprot.writeFieldEnd();
    }
    if (this.optional_int_field != null) {
      if (isSetOptional_int_field()) {
        oprot.writeFieldBegin(OPTIONAL_INT_FIELD_FIELD_DESC);
        oprot.writeI32(this.optional_int_field);
        oprot.writeFieldEnd();
      }
    }
    if (this.optional_bool_field != null) {
      if (isSetOptional_bool_field()) {
        oprot.writeFieldBegin(OPTIONAL_BOOL_FIELD_FIELD_DESC);
        oprot.writeBool(this.optional_bool_field);
        oprot.writeFieldEnd();
      }
    }
    if (this.optional_list_field != null) {
      if (isSetOptional_list_field()) {
        oprot.writeFieldBegin(OPTIONAL_LIST_FIELD_FIELD_DESC);
        {
          oprot.writeListBegin(new TList(TType.I32, this.optional_list_field.size()));
          for (Integer _iter7 : this.optional_list_field)          {
            oprot.writeI32(_iter7);
          }
          oprot.writeListEnd();
        }
        oprot.writeFieldEnd();
      }
    }
    if (this.optional_struct_field != null) {
      if (isSetOptional_struct_field()) {
        oprot.writeFieldBegin(OPTIONAL_STRUCT_FIELD_FIELD_DESC);
        this.optional_struct_field.write(oprot);
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
    return TBaseHelper.toStringHelper(this, indent, prettyPrint);
  }

  public void validate() throws TException {
    // check for required fields
  }

}

