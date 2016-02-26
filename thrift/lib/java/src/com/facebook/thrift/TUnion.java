package com.facebook.thrift;

import com.facebook.thrift.protocol.TField;
import com.facebook.thrift.protocol.TProtocol;
import com.facebook.thrift.protocol.TProtocolException;
import com.facebook.thrift.protocol.TStruct;

@SuppressWarnings("serial")
public abstract class TUnion<Me extends TUnion<Me>> implements TBase {

  protected Object value_;
  protected int setField_;

  protected TUnion() {
    setField_ = 0;
    value_ = null;
  }

  protected TUnion(int setField, Object value) {
    setFieldValue(setField, value);
  }

  protected TUnion(TUnion<Me> other) {
    if (!other.getClass().equals(this.getClass())) {
      throw new ClassCastException();
    }
    setField_ = other.setField_;
    value_ = TBaseHelper.deepCopyUnchecked(other.value_);
  }

  public int getSetField() {
    return setField_;
  }

  public Object getFieldValue() {
    return value_;
  }

  public Object getFieldValue(int fieldId) {
    if (fieldId != setField_) {
      throw new IllegalArgumentException("Cannot get the value of field " + fieldId + " because union's set field is " + setField_);
    }

    return getFieldValue();
  }

  public boolean isSet() {
    return setField_ != 0;
  }

  public boolean isSet(int fieldId) {
    return setField_ == fieldId;
  }

  public void read(TProtocol iprot) throws TException {
    setField_ = 0;
    value_ = null;

    iprot.readStructBegin();

    TField field = iprot.readFieldBegin();

    value_ = readValue(iprot, field);
    if (value_ != null) {
      setField_ = field.id;
    }

    iprot.readFieldEnd();
    // this is so that we will eat the stop byte. we could put a check here to
    // make sure that it actually *is* the stop byte, but it's faster to do it
    // this way.
    iprot.readFieldBegin();
    iprot.readFieldEnd();
    iprot.readStructEnd();
  }

  public void setFieldValue(int fieldId, Object value) {
    checkType((short)fieldId, value);
    setField_ = (short)fieldId;
    value_ = value;
  }

  public void write(TProtocol oprot) throws TException {
    if (getSetField() == 0 || getFieldValue() == null) {
      throw new TProtocolException("Cannot write a TUnion with no set value!");
    }
    oprot.writeStructBegin(getStructDesc());
    oprot.writeFieldBegin(getFieldDesc(setField_));
    writeValue(oprot, (short)setField_, value_);
    oprot.writeFieldEnd();
    oprot.writeFieldStop();
    oprot.writeStructEnd();
  }

  /**
   * Implementation should be generated so that we can efficiently type check
   * various values.
   * @param setField
   * @param value
   */
  protected abstract void checkType(short setField, Object value) throws ClassCastException;

  /**
   * Implementation should be generated to read the right stuff from the wire
   * based on the field header.
   * @param field
   * @return
   */
  protected abstract Object readValue(TProtocol iprot, TField field) throws TException;

  protected abstract void writeValue(TProtocol oprot, short setField, Object value) throws TException;

  protected abstract TStruct getStructDesc();

  protected abstract TField getFieldDesc(int setField);

  public abstract Me deepCopy();

  protected int compareToImpl(Me other) {
    if (other == null) {
      // See docs for java.lang.Comparable
      throw new NullPointerException();
    }

    if (other == this) {
      return 0;
    }

    int lastComparison = TBaseHelper.compareTo(this.setField_, other.setField_);
    if (lastComparison != 0) {
      return lastComparison;
    }

    return TBaseHelper.compareToUnchecked(this, other);
  }

  protected boolean equalsNobinaryImpl(Me other) {
    if (other == null || this.getClass() != other.getClass()) {
      return false;
    }
    if (other == this) {
      return true;
    }
    if (this.getSetField() != other.getSetField()) {
      return false;
    }
    if (this.getFieldValue() == null || other.getFieldValue() == null) {
      return this.getFieldValue() == null && other.getFieldValue() == null;
    }
    return TBaseHelper.equalsNobinaryUnchecked(this.getFieldValue(), other.getFieldValue());
  }

  protected boolean equalsSlowImpl(Me other) {
    if (other == null || this.getClass() != other.getClass()) {
      return false;
    }
    if (other == this) {
      return true;
    }
    if (this.getSetField() != other.getSetField()) {
      return false;
    }
    if (this.getFieldValue() == null || other.getFieldValue() == null) {
      return this.getFieldValue() == null && other.getFieldValue() == null;
    }
    return TBaseHelper.equalsSlowUnchecked(this.getFieldValue(), other.getFieldValue());
  }

  @Override
  public String toString() {
    Object v = getFieldValue();
    String vStr = null;
    if (v instanceof byte[]) {
      vStr = bytesToStr((byte[])v);
    } else {
      vStr = v.toString();
    }
    return "<" + this.getClass().getSimpleName() + " " + getFieldDesc(getSetField()).name + ":" + vStr + ">";
  }

  private static String bytesToStr(byte[] bytes) {
    StringBuilder sb = new StringBuilder();
    int size = Math.min(bytes.length, 128);
    for (int i = 0; i < size; i++) {
      if (i != 0) {
        sb.append(" ");
      }
      String digit = Integer.toHexString(bytes[i]);
      sb.append(digit.length() > 1 ? digit : "0" + digit);
    }
    if (bytes.length > 128) {
      sb.append(" ...");
    }
    return sb.toString();
  }
}
