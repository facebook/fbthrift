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
public class StructWithRefAndAnnotCppNoexceptMoveCtor implements TBase, java.io.Serializable, Cloneable {
  private static final TStruct STRUCT_DESC = new TStruct("StructWithRefAndAnnotCppNoexceptMoveCtor");
  private static final TField DEF_FIELD_FIELD_DESC = new TField("def_field", TType.STRUCT, (short)1);

  public final Empty def_field;
  public static final int DEF_FIELD = 1;

  public StructWithRefAndAnnotCppNoexceptMoveCtor(
      Empty def_field) {
    this.def_field = def_field;
  }

  /**
   * Performs a deep copy on <i>other</i>.
   */
  public StructWithRefAndAnnotCppNoexceptMoveCtor(StructWithRefAndAnnotCppNoexceptMoveCtor other) {
    if (other.isSetDef_field()) {
      this.def_field = TBaseHelper.deepCopy(other.def_field);
    } else {
      this.def_field = null;
    }
  }

  public StructWithRefAndAnnotCppNoexceptMoveCtor deepCopy() {
    return new StructWithRefAndAnnotCppNoexceptMoveCtor(this);
  }

  public Empty getDef_field() {
    return this.def_field;
  }

  // Returns true if field def_field is set (has been assigned a value) and false otherwise
  public boolean isSetDef_field() {
    return this.def_field != null;
  }

  @Override
  public boolean equals(Object _that) {
    if (_that == null)
      return false;
    if (this == _that)
      return true;
    if (!(_that instanceof StructWithRefAndAnnotCppNoexceptMoveCtor))
      return false;
    StructWithRefAndAnnotCppNoexceptMoveCtor that = (StructWithRefAndAnnotCppNoexceptMoveCtor)_that;

    if (!TBaseHelper.equalsNobinary(this.isSetDef_field(), that.isSetDef_field(), this.def_field, that.def_field)) { return false; }

    return true;
  }

  @Override
  public int hashCode() {
    return Arrays.deepHashCode(new Object[] {def_field});
  }

  // This is required to satisfy the TBase interface, but can't be implemented on immutable struture.
  public void read(TProtocol iprot) throws TException {
    throw new TException("unimplemented in android immutable structure");
  }

  public static StructWithRefAndAnnotCppNoexceptMoveCtor deserialize(TProtocol iprot) throws TException {
    Empty tmp_def_field = null;
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
        case DEF_FIELD:
          if (__field.type == TType.STRUCT) {
            tmp_def_field = Empty.deserialize(iprot);
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

    StructWithRefAndAnnotCppNoexceptMoveCtor _that;
    _that = new StructWithRefAndAnnotCppNoexceptMoveCtor(
      tmp_def_field
    );
    _that.validate();
    return _that;
  }

  public void write(TProtocol oprot) throws TException {
    validate();

    oprot.writeStructBegin(STRUCT_DESC);
    if (this.def_field != null) {
      oprot.writeFieldBegin(DEF_FIELD_FIELD_DESC);
      this.def_field.write(oprot);
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

