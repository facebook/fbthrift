/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */

package test.fixtures.injectMetadataFields;

import com.facebook.swift.codec.*;
import com.facebook.swift.codec.ThriftField.Requiredness;
import com.facebook.swift.codec.ThriftField.Recursiveness;
import com.google.common.collect.*;
import java.util.*;
import javax.annotation.Nullable;
import org.apache.thrift.*;
import org.apache.thrift.transport.*;
import org.apache.thrift.protocol.*;
import static com.google.common.base.MoreObjects.toStringHelper;
import static com.google.common.base.MoreObjects.ToStringHelper;

@SwiftGenerated
@com.facebook.swift.codec.ThriftStruct(value="FieldsInjectedWithIncludedStruct", builder=FieldsInjectedWithIncludedStruct.Builder.class)
public final class FieldsInjectedWithIncludedStruct implements com.facebook.thrift.payload.ThriftSerializable {
    @ThriftConstructor
    public FieldsInjectedWithIncludedStruct(
        @com.facebook.swift.codec.ThriftField(value=1, name="string_field", requiredness=Requiredness.NONE) final String stringField,
        @com.facebook.swift.codec.ThriftField(value=-1100, name="injected_field", isLegacyId=true, requiredness=Requiredness.NONE) final String injectedField,
        @com.facebook.swift.codec.ThriftField(value=-1101, name="injected_structured_annotation_field", isLegacyId=true, requiredness=Requiredness.OPTIONAL) final String injectedStructuredAnnotationField,
        @com.facebook.swift.codec.ThriftField(value=-1102, name="injected_unstructured_annotation_field", isLegacyId=true, requiredness=Requiredness.OPTIONAL) final String injectedUnstructuredAnnotationField
    ) {
        this.stringField = stringField;
        this.injectedField = injectedField;
        this.injectedStructuredAnnotationField = injectedStructuredAnnotationField;
        this.injectedUnstructuredAnnotationField = injectedUnstructuredAnnotationField;
    }
    
    @ThriftConstructor
    protected FieldsInjectedWithIncludedStruct() {
      this.stringField = null;
      this.injectedField = null;
      this.injectedStructuredAnnotationField = null;
      this.injectedUnstructuredAnnotationField = null;
    }
    
    public static Builder builder() {
      return new Builder();
    }

    public static Builder builder(FieldsInjectedWithIncludedStruct other) {
      return new Builder(other);
    }

    public static class Builder {
        private String stringField = null;
        private String injectedField = null;
        private String injectedStructuredAnnotationField = null;
        private String injectedUnstructuredAnnotationField = null;
    
        @com.facebook.swift.codec.ThriftField(value=1, name="string_field", requiredness=Requiredness.NONE)
        public Builder setStringField(String stringField) {
            this.stringField = stringField;
            return this;
        }
    
        public String getStringField() { return stringField; }
    
            @com.facebook.swift.codec.ThriftField(value=-1100, name="injected_field", isLegacyId=true, requiredness=Requiredness.NONE)
        public Builder setInjectedField(String injectedField) {
            this.injectedField = injectedField;
            return this;
        }
    
        public String getInjectedField() { return injectedField; }
    
            @com.facebook.swift.codec.ThriftField(value=-1101, name="injected_structured_annotation_field", isLegacyId=true, requiredness=Requiredness.OPTIONAL)
        public Builder setInjectedStructuredAnnotationField(String injectedStructuredAnnotationField) {
            this.injectedStructuredAnnotationField = injectedStructuredAnnotationField;
            return this;
        }
    
        public String getInjectedStructuredAnnotationField() { return injectedStructuredAnnotationField; }
    
            @com.facebook.swift.codec.ThriftField(value=-1102, name="injected_unstructured_annotation_field", isLegacyId=true, requiredness=Requiredness.OPTIONAL)
        public Builder setInjectedUnstructuredAnnotationField(String injectedUnstructuredAnnotationField) {
            this.injectedUnstructuredAnnotationField = injectedUnstructuredAnnotationField;
            return this;
        }
    
        public String getInjectedUnstructuredAnnotationField() { return injectedUnstructuredAnnotationField; }
    
        public Builder() { }
        public Builder(FieldsInjectedWithIncludedStruct other) {
            this.stringField = other.stringField;
            this.injectedField = other.injectedField;
            this.injectedStructuredAnnotationField = other.injectedStructuredAnnotationField;
            this.injectedUnstructuredAnnotationField = other.injectedUnstructuredAnnotationField;
        }
    
        @ThriftConstructor
        public FieldsInjectedWithIncludedStruct build() {
            FieldsInjectedWithIncludedStruct result = new FieldsInjectedWithIncludedStruct (
                this.stringField,
                this.injectedField,
                this.injectedStructuredAnnotationField,
                this.injectedUnstructuredAnnotationField
            );
            return result;
        }
    }
        
    public static final Map<String, Integer> NAMES_TO_IDS = new HashMap<>();
    public static final Map<String, Integer> THRIFT_NAMES_TO_IDS = new HashMap<>();
    public static final Map<Integer, TField> FIELD_METADATA = new HashMap<>();
    private static final TStruct STRUCT_DESC = new TStruct("FieldsInjectedWithIncludedStruct");
    private final String stringField;
    public static final int _STRING_FIELD = 1;
    private static final TField STRING_FIELD_FIELD_DESC = new TField("string_field", TType.STRING, (short)1);
        private final String injectedField;
    public static final int _INJECTED_FIELD = -1100;
    private static final TField INJECTED_FIELD_FIELD_DESC = new TField("injected_field", TType.STRING, (short)-1100);
        private final String injectedStructuredAnnotationField;
    public static final int _INJECTED_STRUCTURED_ANNOTATION_FIELD = -1101;
    private static final TField INJECTED_STRUCTURED_ANNOTATION_FIELD_FIELD_DESC = new TField("injected_structured_annotation_field", TType.STRING, (short)-1101);
        private final String injectedUnstructuredAnnotationField;
    public static final int _INJECTED_UNSTRUCTURED_ANNOTATION_FIELD = -1102;
    private static final TField INJECTED_UNSTRUCTURED_ANNOTATION_FIELD_FIELD_DESC = new TField("injected_unstructured_annotation_field", TType.STRING, (short)-1102);
    static {
      NAMES_TO_IDS.put("stringField", 1);
      THRIFT_NAMES_TO_IDS.put("string_field", 1);
      FIELD_METADATA.put(1, STRING_FIELD_FIELD_DESC);
      NAMES_TO_IDS.put("injectedField", -1100);
      THRIFT_NAMES_TO_IDS.put("injected_field", -1100);
      FIELD_METADATA.put(-1100, INJECTED_FIELD_FIELD_DESC);
      NAMES_TO_IDS.put("injectedStructuredAnnotationField", -1101);
      THRIFT_NAMES_TO_IDS.put("injected_structured_annotation_field", -1101);
      FIELD_METADATA.put(-1101, INJECTED_STRUCTURED_ANNOTATION_FIELD_FIELD_DESC);
      NAMES_TO_IDS.put("injectedUnstructuredAnnotationField", -1102);
      THRIFT_NAMES_TO_IDS.put("injected_unstructured_annotation_field", -1102);
      FIELD_METADATA.put(-1102, INJECTED_UNSTRUCTURED_ANNOTATION_FIELD_FIELD_DESC);
    }
    
    @Nullable
    @com.facebook.swift.codec.ThriftField(value=1, name="string_field", requiredness=Requiredness.NONE)
    public String getStringField() { return stringField; }
    
    
    @Nullable
    @com.facebook.swift.codec.ThriftField(value=-1100, name="injected_field", isLegacyId=true, requiredness=Requiredness.NONE)
    public String getInjectedField() { return injectedField; }
    
    
    @Nullable
    @com.facebook.swift.codec.ThriftField(value=-1101, name="injected_structured_annotation_field", isLegacyId=true, requiredness=Requiredness.OPTIONAL)
    public String getInjectedStructuredAnnotationField() { return injectedStructuredAnnotationField; }
    
    
    @Nullable
    @com.facebook.swift.codec.ThriftField(value=-1102, name="injected_unstructured_annotation_field", isLegacyId=true, requiredness=Requiredness.OPTIONAL)
    public String getInjectedUnstructuredAnnotationField() { return injectedUnstructuredAnnotationField; }
    
    @java.lang.Override
    public String toString() {
        ToStringHelper helper = toStringHelper(this);
        helper.add("stringField", stringField);
        helper.add("injectedField", injectedField);
        helper.add("injectedStructuredAnnotationField", injectedStructuredAnnotationField);
        helper.add("injectedUnstructuredAnnotationField", injectedUnstructuredAnnotationField);
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
    
        FieldsInjectedWithIncludedStruct other = (FieldsInjectedWithIncludedStruct)o;
    
        return
            Objects.equals(stringField, other.stringField) &&
            Objects.equals(injectedField, other.injectedField) &&
            Objects.equals(injectedStructuredAnnotationField, other.injectedStructuredAnnotationField) &&
            Objects.equals(injectedUnstructuredAnnotationField, other.injectedUnstructuredAnnotationField) &&
            true;
    }
    
    @java.lang.Override
    public int hashCode() {
        return Arrays.deepHashCode(new java.lang.Object[] {
            stringField,
            injectedField,
            injectedStructuredAnnotationField,
            injectedUnstructuredAnnotationField
        });
    }
    
    
    public static com.facebook.thrift.payload.Reader<FieldsInjectedWithIncludedStruct> asReader() {
      return FieldsInjectedWithIncludedStruct::read0;
    }
    
    public static FieldsInjectedWithIncludedStruct read0(TProtocol oprot) throws TException {
      TField __field;
      oprot.readStructBegin(FieldsInjectedWithIncludedStruct.NAMES_TO_IDS, FieldsInjectedWithIncludedStruct.THRIFT_NAMES_TO_IDS, FieldsInjectedWithIncludedStruct.FIELD_METADATA);
      FieldsInjectedWithIncludedStruct.Builder builder = new FieldsInjectedWithIncludedStruct.Builder();
      while (true) {
        __field = oprot.readFieldBegin();
        if (__field.type == TType.STOP) { break; }
        switch (__field.id) {
        case _STRING_FIELD:
          if (__field.type == TType.STRING) {
            String stringField = oprot.readString();
            builder.setStringField(stringField);
          } else {
            TProtocolUtil.skip(oprot, __field.type);
          }
          break;
        case _INJECTED_FIELD:
          if (__field.type == TType.STRING) {
            String injectedField = oprot.readString();
            builder.setInjectedField(injectedField);
          } else {
            TProtocolUtil.skip(oprot, __field.type);
          }
          break;
        case _INJECTED_STRUCTURED_ANNOTATION_FIELD:
          if (__field.type == TType.STRING) {
            String injectedStructuredAnnotationField = oprot.readString();
            builder.setInjectedStructuredAnnotationField(injectedStructuredAnnotationField);
          } else {
            TProtocolUtil.skip(oprot, __field.type);
          }
          break;
        case _INJECTED_UNSTRUCTURED_ANNOTATION_FIELD:
          if (__field.type == TType.STRING) {
            String injectedUnstructuredAnnotationField = oprot.readString();
            builder.setInjectedUnstructuredAnnotationField(injectedUnstructuredAnnotationField);
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
      if (stringField != null) {
        oprot.writeFieldBegin(STRING_FIELD_FIELD_DESC);
        oprot.writeString(this.stringField);
        oprot.writeFieldEnd();
      }
      if (injectedField != null) {
        oprot.writeFieldBegin(INJECTED_FIELD_FIELD_DESC);
        oprot.writeString(this.injectedField);
        oprot.writeFieldEnd();
      }
      if (injectedStructuredAnnotationField != null) {
        oprot.writeFieldBegin(INJECTED_STRUCTURED_ANNOTATION_FIELD_FIELD_DESC);
        oprot.writeString(this.injectedStructuredAnnotationField);
        oprot.writeFieldEnd();
      }
      if (injectedUnstructuredAnnotationField != null) {
        oprot.writeFieldBegin(INJECTED_UNSTRUCTURED_ANNOTATION_FIELD_FIELD_DESC);
        oprot.writeString(this.injectedUnstructuredAnnotationField);
        oprot.writeFieldEnd();
      }
      oprot.writeFieldStop();
      oprot.writeStructEnd();
    }
    
    private static class _FieldsInjectedWithIncludedStructLazy {
        private static final FieldsInjectedWithIncludedStruct _DEFAULT = new FieldsInjectedWithIncludedStruct.Builder().build();
    }
    
    public static FieldsInjectedWithIncludedStruct defaultInstance() {
        return  _FieldsInjectedWithIncludedStructLazy._DEFAULT;
    }
}
