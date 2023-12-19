/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
package test.fixtures.enums;

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
public class SomeStruct implements TBase, java.io.Serializable, Cloneable {
  private static final TStruct STRUCT_DESC = new TStruct("SomeStruct");
  private static final TField REASONABLE_FIELD_DESC = new TField("reasonable", TType.I32, (short)1);
  private static final TField FINE_FIELD_DESC = new TField("fine", TType.I32, (short)2);
  private static final TField QUESTIONABLE_FIELD_DESC = new TField("questionable", TType.I32, (short)3);
  private static final TField TAGS_FIELD_DESC = new TField("tags", TType.SET, (short)4);

  /**
   * 
   * @see Metasyntactic
   */
  public final Metasyntactic reasonable;
  /**
   * 
   * @see Metasyntactic
   */
  public final Metasyntactic fine;
  /**
   * 
   * @see Metasyntactic
   */
  public final Metasyntactic questionable;
  public final Set<Integer> tags;
  public static final int REASONABLE = 1;
  public static final int FINE = 2;
  public static final int QUESTIONABLE = 3;
  public static final int TAGS = 4;

  public SomeStruct(
      Metasyntactic reasonable,
      Metasyntactic fine,
      Metasyntactic questionable,
      Set<Integer> tags) {
    this.reasonable = reasonable;
    this.fine = fine;
    this.questionable = questionable;
    this.tags = tags;
  }

  /**
   * Performs a deep copy on <i>other</i>.
   */
  public SomeStruct(SomeStruct other) {
    if (other.isSetReasonable()) {
      this.reasonable = TBaseHelper.deepCopy(other.reasonable);
    } else {
      this.reasonable = null;
    }
    if (other.isSetFine()) {
      this.fine = TBaseHelper.deepCopy(other.fine);
    } else {
      this.fine = null;
    }
    if (other.isSetQuestionable()) {
      this.questionable = TBaseHelper.deepCopy(other.questionable);
    } else {
      this.questionable = null;
    }
    if (other.isSetTags()) {
      this.tags = TBaseHelper.deepCopy(other.tags);
    } else {
      this.tags = null;
    }
  }

  public SomeStruct deepCopy() {
    return new SomeStruct(this);
  }

  /**
   * 
   * @see Metasyntactic
   */
  public Metasyntactic getReasonable() {
    return this.reasonable;
  }

  // Returns true if field reasonable is set (has been assigned a value) and false otherwise
  public boolean isSetReasonable() {
    return this.reasonable != null;
  }

  /**
   * 
   * @see Metasyntactic
   */
  public Metasyntactic getFine() {
    return this.fine;
  }

  // Returns true if field fine is set (has been assigned a value) and false otherwise
  public boolean isSetFine() {
    return this.fine != null;
  }

  /**
   * 
   * @see Metasyntactic
   */
  public Metasyntactic getQuestionable() {
    return this.questionable;
  }

  // Returns true if field questionable is set (has been assigned a value) and false otherwise
  public boolean isSetQuestionable() {
    return this.questionable != null;
  }

  public Set<Integer> getTags() {
    return this.tags;
  }

  // Returns true if field tags is set (has been assigned a value) and false otherwise
  public boolean isSetTags() {
    return this.tags != null;
  }

  @Override
  public boolean equals(Object _that) {
    if (_that == null)
      return false;
    if (this == _that)
      return true;
    if (!(_that instanceof SomeStruct))
      return false;
    SomeStruct that = (SomeStruct)_that;

    if (!TBaseHelper.equalsNobinary(this.isSetReasonable(), that.isSetReasonable(), this.reasonable, that.reasonable)) { return false; }

    if (!TBaseHelper.equalsNobinary(this.isSetFine(), that.isSetFine(), this.fine, that.fine)) { return false; }

    if (!TBaseHelper.equalsNobinary(this.isSetQuestionable(), that.isSetQuestionable(), this.questionable, that.questionable)) { return false; }

    if (!TBaseHelper.equalsNobinary(this.isSetTags(), that.isSetTags(), this.tags, that.tags)) { return false; }

    return true;
  }

  @Override
  public int hashCode() {
    return Arrays.deepHashCode(new Object[] {reasonable, fine, questionable, tags});
  }

  // This is required to satisfy the TBase interface, but can't be implemented on immutable struture.
  public void read(TProtocol iprot) throws TException {
    throw new TException("unimplemented in android immutable structure");
  }

  public static SomeStruct deserialize(TProtocol iprot) throws TException {
    Metasyntactic tmp_reasonable = null;
    Metasyntactic tmp_fine = null;
    Metasyntactic tmp_questionable = null;
    Set<Integer> tmp_tags = null;
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
        case REASONABLE:
          if (__field.type == TType.I32) {
            tmp_reasonable = Metasyntactic.findByValue(iprot.readI32());
          } else {
            TProtocolUtil.skip(iprot, __field.type);
          }
          break;
        case FINE:
          if (__field.type == TType.I32) {
            tmp_fine = Metasyntactic.findByValue(iprot.readI32());
          } else {
            TProtocolUtil.skip(iprot, __field.type);
          }
          break;
        case QUESTIONABLE:
          if (__field.type == TType.I32) {
            tmp_questionable = Metasyntactic.findByValue(iprot.readI32());
          } else {
            TProtocolUtil.skip(iprot, __field.type);
          }
          break;
        case TAGS:
          if (__field.type == TType.SET) {
            {
              TSet _set0 = iprot.readSetBegin();
              tmp_tags = new HashSet<Integer>(Math.max(0, 2*_set0.size));
              for (int _i1 = 0; 
                   (_set0.size < 0) ? iprot.peekSet() : (_i1 < _set0.size); 
                   ++_i1)
              {
                Integer _elem2;
                _elem2 = iprot.readI32();
                tmp_tags.add(_elem2);
              }
              iprot.readSetEnd();
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

    SomeStruct _that;
    _that = new SomeStruct(
      tmp_reasonable
      ,tmp_fine
      ,tmp_questionable
      ,tmp_tags
    );
    _that.validate();
    return _that;
  }

  public void write(TProtocol oprot) throws TException {
    validate();

    oprot.writeStructBegin(STRUCT_DESC);
    if (this.reasonable != null) {
      oprot.writeFieldBegin(REASONABLE_FIELD_DESC);
      oprot.writeI32(this.reasonable == null ? 0 : this.reasonable.getValue());
      oprot.writeFieldEnd();
    }
    if (this.fine != null) {
      oprot.writeFieldBegin(FINE_FIELD_DESC);
      oprot.writeI32(this.fine == null ? 0 : this.fine.getValue());
      oprot.writeFieldEnd();
    }
    if (this.questionable != null) {
      oprot.writeFieldBegin(QUESTIONABLE_FIELD_DESC);
      oprot.writeI32(this.questionable == null ? 0 : this.questionable.getValue());
      oprot.writeFieldEnd();
    }
    if (this.tags != null) {
      oprot.writeFieldBegin(TAGS_FIELD_DESC);
      {
        oprot.writeSetBegin(new TSet(TType.I32, this.tags.size()));
        for (Integer _iter3 : this.tags)        {
          oprot.writeI32(_iter3);
        }
        oprot.writeSetEnd();
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
