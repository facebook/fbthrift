/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */

package test.fixtures.basic_swift_bean;

import com.facebook.swift.codec.*;
import com.facebook.swift.codec.ThriftField.Requiredness;
import com.facebook.swift.codec.ThriftField.Recursiveness;
import com.google.common.collect.*;
import java.util.*;
import javax.annotation.Nullable;
import org.apache.thrift.*;
import org.apache.thrift.TException;
import org.apache.thrift.transport.*;
import org.apache.thrift.protocol.*;
import org.apache.thrift.protocol.TProtocol;
import static com.google.common.base.MoreObjects.toStringHelper;
import static com.google.common.base.MoreObjects.ToStringHelper;

@SwiftGenerated
@com.facebook.swift.codec.ThriftStruct(value="MyStruct1", builder=MyStruct1.Builder.class)
public final class MyStruct1 implements com.facebook.thrift.payload.ThriftSerializable {
    @ThriftConstructor
    public MyStruct1(
        @com.facebook.swift.codec.ThriftField(value=1, name="MyIntField", requiredness=Requiredness.NONE) final long myIntField,
        @com.facebook.swift.codec.ThriftField(value=2, name="MyStringField", requiredness=Requiredness.NONE) final String myStringField,
        @com.facebook.swift.codec.ThriftField(value=3, name="MyDataField", requiredness=Requiredness.NONE) final test.fixtures.basic_swift_bean.MyDataItem myDataField,
        @com.facebook.swift.codec.ThriftField(value=4, name="major", requiredness=Requiredness.NONE) final long major
    ) {
        this.myIntField = myIntField;
        this.myStringField = myStringField;
        this.myDataField = myDataField;
        this.major = major;
    }
    
    @ThriftConstructor
    protected MyStruct1() {
      this.myIntField = 0L;
      this.myStringField = null;
      this.myDataField = null;
      this.major = 0L;
    }

    public static Builder builder() {
      return new Builder();
    }

    public static Builder builder(MyStruct1 other) {
      return new Builder(other);
    }

    public static class Builder {
        private long myIntField = 0L;
        private String myStringField = null;
        private test.fixtures.basic_swift_bean.MyDataItem myDataField = null;
        private long major = 0L;
    
        @com.facebook.swift.codec.ThriftField(value=1, name="MyIntField", requiredness=Requiredness.NONE)    public Builder setMyIntField(long myIntField) {
            this.myIntField = myIntField;
            return this;
        }
    
        public long getMyIntField() { return myIntField; }
    
            @com.facebook.swift.codec.ThriftField(value=2, name="MyStringField", requiredness=Requiredness.NONE)    public Builder setMyStringField(String myStringField) {
            this.myStringField = myStringField;
            return this;
        }
    
        public String getMyStringField() { return myStringField; }
    
            @com.facebook.swift.codec.ThriftField(value=3, name="MyDataField", requiredness=Requiredness.NONE)    public Builder setMyDataField(test.fixtures.basic_swift_bean.MyDataItem myDataField) {
            this.myDataField = myDataField;
            return this;
        }
    
        public test.fixtures.basic_swift_bean.MyDataItem getMyDataField() { return myDataField; }
    
            @com.facebook.swift.codec.ThriftField(value=4, name="major", requiredness=Requiredness.NONE)    public Builder setMajor(long major) {
            this.major = major;
            return this;
        }
    
        public long getMajor() { return major; }
    
        public Builder() { }
        public Builder(MyStruct1 other) {
            this.myIntField = other.myIntField;
            this.myStringField = other.myStringField;
            this.myDataField = other.myDataField;
            this.major = other.major;
        }
    
        @ThriftConstructor
        public MyStruct1 build() {
            MyStruct1 result = new MyStruct1 (
                this.myIntField,
                this.myStringField,
                this.myDataField,
                this.major
            );
            return result;
        }
    }
    
    public static final Map<String, Integer> NAMES_TO_IDS = new HashMap<>();
    public static final Map<String, Integer> THRIFT_NAMES_TO_IDS = new HashMap<>();
    public static final Map<Integer, TField> FIELD_METADATA = new HashMap<>();
    private static final TStruct STRUCT_DESC = new TStruct("MyStruct1");
    private final long myIntField;
    public static final int _MYINTFIELD = 1;
    private static final TField MY_INT_FIELD_FIELD_DESC = new TField("MyIntField", TType.I64, (short)1);
        private final String myStringField;
    public static final int _MYSTRINGFIELD = 2;
    private static final TField MY_STRING_FIELD_FIELD_DESC = new TField("MyStringField", TType.STRING, (short)2);
        private final test.fixtures.basic_swift_bean.MyDataItem myDataField;
    public static final int _MYDATAFIELD = 3;
    private static final TField MY_DATA_FIELD_FIELD_DESC = new TField("MyDataField", TType.STRUCT, (short)3);
        private final long major;
    public static final int _MAJOR = 4;
    private static final TField MAJOR_FIELD_DESC = new TField("major", TType.I64, (short)4);
    static {
      NAMES_TO_IDS.put("myIntField", 1);
      THRIFT_NAMES_TO_IDS.put("MyIntField", 1);
      FIELD_METADATA.put(1, MY_INT_FIELD_FIELD_DESC);
      NAMES_TO_IDS.put("myStringField", 2);
      THRIFT_NAMES_TO_IDS.put("MyStringField", 2);
      FIELD_METADATA.put(2, MY_STRING_FIELD_FIELD_DESC);
      NAMES_TO_IDS.put("myDataField", 3);
      THRIFT_NAMES_TO_IDS.put("MyDataField", 3);
      FIELD_METADATA.put(3, MY_DATA_FIELD_FIELD_DESC);
      NAMES_TO_IDS.put("major", 4);
      THRIFT_NAMES_TO_IDS.put("major", 4);
      FIELD_METADATA.put(4, MAJOR_FIELD_DESC);
    }
    
    
    @com.facebook.swift.codec.ThriftField(value=1, name="MyIntField", requiredness=Requiredness.NONE)
    public long getMyIntField() { return myIntField; }

    
    @Nullable
    @com.facebook.swift.codec.ThriftField(value=2, name="MyStringField", requiredness=Requiredness.NONE)
    public String getMyStringField() { return myStringField; }

    
    @Nullable
    @com.facebook.swift.codec.ThriftField(value=3, name="MyDataField", requiredness=Requiredness.NONE)
    public test.fixtures.basic_swift_bean.MyDataItem getMyDataField() { return myDataField; }

    
    
    @com.facebook.swift.codec.ThriftField(value=4, name="major", requiredness=Requiredness.NONE)
    public long getMajor() { return major; }

    @java.lang.Override
    public String toString() {
        ToStringHelper helper = toStringHelper(this);
        helper.add("myIntField", myIntField);
        helper.add("myStringField", myStringField);
        helper.add("myDataField", myDataField);
        helper.add("major", major);
        return helper.toString();
    }

    @java.lang.Override
    public boolean equals(java.lang.Object o) {
        if (this == o) {
            return true;
        }
        if (o == null || getClass() != o.getClass()) {
            return false;
        }
    
        MyStruct1 other = (MyStruct1)o;
    
        return
            Objects.equals(myIntField, other.myIntField) &&
            Objects.equals(myStringField, other.myStringField) &&
            Objects.equals(myDataField, other.myDataField) &&
            Objects.equals(major, other.major) &&
            true;
    }

    @java.lang.Override
    public int hashCode() {
        return Arrays.deepHashCode(new java.lang.Object[] {
            myIntField,
            myStringField,
            myDataField,
            major
        });
    }

    
    public static com.facebook.thrift.payload.Reader<MyStruct1> asReader() {
      return MyStruct1::read0;
    }
    
    public static MyStruct1 read0(TProtocol oprot) throws TException {
      TField __field;
      oprot.readStructBegin(MyStruct1.NAMES_TO_IDS, MyStruct1.THRIFT_NAMES_TO_IDS, MyStruct1.FIELD_METADATA);
      MyStruct1.Builder builder = new MyStruct1.Builder();
      while (true) {
        __field = oprot.readFieldBegin();
        if (__field.type == TType.STOP) { break; }
        switch (__field.id) {
        case _MYINTFIELD:
          if (__field.type == TType.I64) {
            long myIntField = oprot.readI64();
            builder.setMyIntField(myIntField);
          } else {
            TProtocolUtil.skip(oprot, __field.type);
          }
          break;
        case _MYSTRINGFIELD:
          if (__field.type == TType.STRING) {
            String myStringField = oprot.readString();
            builder.setMyStringField(myStringField);
          } else {
            TProtocolUtil.skip(oprot, __field.type);
          }
          break;
        case _MYDATAFIELD:
          if (__field.type == TType.STRUCT) {
            test.fixtures.basic_swift_bean.MyDataItem myDataField = test.fixtures.basic_swift_bean.MyDataItem.read0(oprot);
            builder.setMyDataField(myDataField);
          } else {
            TProtocolUtil.skip(oprot, __field.type);
          }
          break;
        case _MAJOR:
          if (__field.type == TType.I64) {
            long major = oprot.readI64();
            builder.setMajor(major);
          } else {
            TProtocolUtil.skip(oprot, __field.type);
          }
          break;
        default:
          TProtocolUtil.skip(oprot, __field.type);
          break;
        }
        oprot.readFieldEnd();
      }
      oprot.readStructEnd();
      return builder.build();
    }

    public void write0(TProtocol oprot) throws TException {
      oprot.writeStructBegin(STRUCT_DESC);
      oprot.writeFieldBegin(MY_INT_FIELD_FIELD_DESC);
      oprot.writeI64(this.myIntField);
      oprot.writeFieldEnd();
      if (myStringField != null) {
        oprot.writeFieldBegin(MY_STRING_FIELD_FIELD_DESC);
        oprot.writeString(this.myStringField);
        oprot.writeFieldEnd();
      }
      if (myDataField != null) {
        oprot.writeFieldBegin(MY_DATA_FIELD_FIELD_DESC);
        this.myDataField.write0(oprot);
        oprot.writeFieldEnd();
      }
      oprot.writeFieldBegin(MAJOR_FIELD_DESC);
      oprot.writeI64(this.major);
      oprot.writeFieldEnd();
      oprot.writeFieldStop();
      oprot.writeStructEnd();
    }

    private static class _MyStruct1Lazy {
        private static final MyStruct1 _DEFAULT = new MyStruct1.Builder().build();
    }
    
    public static MyStruct1 defaultInstance() {
        return  _MyStruct1Lazy._DEFAULT;
    }
}
