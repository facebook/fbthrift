/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
package com.facebook.thrift.annotation.rust_deprecated;

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
public class Derive implements TBase, java.io.Serializable, Cloneable {
  private static final TStruct STRUCT_DESC = new TStruct("Derive");
  private static final TField DERIVES_FIELD_DESC = new TField("derives", TType.LIST, (short)1);

  public final List<String> derives;
  public static final int DERIVES = 1;

  public Derive(
      List<String> derives) {
    this.derives = derives;
  }

  /**
   * Performs a deep copy on <i>other</i>.
   */
  public Derive(Derive other) {
    if (other.isSetDerives()) {
      this.derives = TBaseHelper.deepCopy(other.derives);
    } else {
      this.derives = null;
    }
  }

  public Derive deepCopy() {
    return new Derive(this);
  }

  public List<String> getDerives() {
    return this.derives;
  }

  // Returns true if field derives is set (has been assigned a value) and false otherwise
  public boolean isSetDerives() {
    return this.derives != null;
  }

  @Override
  public boolean equals(Object _that) {
    if (_that == null)
      return false;
    if (this == _that)
      return true;
    if (!(_that instanceof Derive))
      return false;
    Derive that = (Derive)_that;

    if (!TBaseHelper.equalsNobinary(this.isSetDerives(), that.isSetDerives(), this.derives, that.derives)) { return false; }

    return true;
  }

  @Override
  public int hashCode() {
    return Arrays.deepHashCode(new Object[] {derives});
  }

  // This is required to satisfy the TBase interface, but can't be implemented on immutable struture.
  public void read(TProtocol iprot) throws TException {
    throw new TException("unimplemented in android immutable structure");
  }

  public static Derive deserialize(TProtocol iprot) throws TException {
    List<String> tmp_derives = null;
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
        case DERIVES:
          if (__field.type == TType.LIST) {
            {
              TList _list0 = iprot.readListBegin();
              tmp_derives = new ArrayList<String>(Math.max(0, _list0.size));
              for (int _i1 = 0; 
                   (_list0.size < 0) ? iprot.peekList() : (_i1 < _list0.size); 
                   ++_i1)
              {
                String _elem2;
                _elem2 = iprot.readString();
                tmp_derives.add(_elem2);
              }
              iprot.readListEnd();
            }
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

    Derive _that;
    _that = new Derive(
      tmp_derives
    );
    _that.validate();
    return _that;
  }

  public void write(TProtocol oprot) throws TException {
    validate();

    oprot.writeStructBegin(STRUCT_DESC);
    if (this.derives != null) {
      oprot.writeFieldBegin(DERIVES_FIELD_DESC);
      {
        oprot.writeListBegin(new TList(TType.STRING, this.derives.size()));
        for (String _iter3 : this.derives)        {
          oprot.writeString(_iter3);
        }
        oprot.writeListEnd();
      }
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
