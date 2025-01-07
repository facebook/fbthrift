/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */

package test.fixtures.sink;

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
@com.facebook.swift.codec.ThriftStruct(value="CompatibleWithKeywordSink", builder=CompatibleWithKeywordSink.Builder.class)
public final class CompatibleWithKeywordSink implements com.facebook.thrift.payload.ThriftSerializable {
    @ThriftConstructor
    public CompatibleWithKeywordSink(
        @com.facebook.swift.codec.ThriftField(value=1, name="sink", requiredness=Requiredness.NONE) final String sink
    ) {
        this.sink = sink;
    }
    
    @ThriftConstructor
    protected CompatibleWithKeywordSink() {
      this.sink = null;
    }

    public static Builder builder() {
      return new Builder();
    }

    public static Builder builder(CompatibleWithKeywordSink other) {
      return new Builder(other);
    }

    public static class Builder {
        private String sink = null;
    
        @com.facebook.swift.codec.ThriftField(value=1, name="sink", requiredness=Requiredness.NONE)    public Builder setSink(String sink) {
            this.sink = sink;
            return this;
        }
    
        public String getSink() { return sink; }
    
        public Builder() { }
        public Builder(CompatibleWithKeywordSink other) {
            this.sink = other.sink;
        }
    
        @ThriftConstructor
        public CompatibleWithKeywordSink build() {
            CompatibleWithKeywordSink result = new CompatibleWithKeywordSink (
                this.sink
            );
            return result;
        }
    }
    
    public static final Map<String, Integer> NAMES_TO_IDS = new HashMap<>();
    public static final Map<String, Integer> THRIFT_NAMES_TO_IDS = new HashMap<>();
    public static final Map<Integer, TField> FIELD_METADATA = new HashMap<>();
    private static final TStruct STRUCT_DESC = new TStruct("CompatibleWithKeywordSink");
    private final String sink;
    public static final int _SINK = 1;
    private static final TField SINK_FIELD_DESC = new TField("sink", TType.STRING, (short)1);
    static {
      NAMES_TO_IDS.put("sink", 1);
      THRIFT_NAMES_TO_IDS.put("sink", 1);
      FIELD_METADATA.put(1, SINK_FIELD_DESC);
    }
    
    @Nullable
    @com.facebook.swift.codec.ThriftField(value=1, name="sink", requiredness=Requiredness.NONE)
    public String getSink() { return sink; }

    @java.lang.Override
    public String toString() {
        ToStringHelper helper = toStringHelper(this);
        helper.add("sink", sink);
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
    
        CompatibleWithKeywordSink other = (CompatibleWithKeywordSink)o;
    
        return
            Objects.equals(sink, other.sink) &&
            true;
    }

    @java.lang.Override
    public int hashCode() {
        return Arrays.deepHashCode(new java.lang.Object[] {
            sink
        });
    }

    
    public static com.facebook.thrift.payload.Reader<CompatibleWithKeywordSink> asReader() {
      return CompatibleWithKeywordSink::read0;
    }
    
    public static CompatibleWithKeywordSink read0(TProtocol oprot) throws TException {
      TField __field;
      oprot.readStructBegin(CompatibleWithKeywordSink.NAMES_TO_IDS, CompatibleWithKeywordSink.THRIFT_NAMES_TO_IDS, CompatibleWithKeywordSink.FIELD_METADATA);
      CompatibleWithKeywordSink.Builder builder = new CompatibleWithKeywordSink.Builder();
      while (true) {
        __field = oprot.readFieldBegin();
        if (__field.type == TType.STOP) { break; }
        switch (__field.id) {
        case _SINK:
          if (__field.type == TType.STRING) {
            String sink = oprot.readString();
            builder.setSink(sink);
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
      if (sink != null) {
        oprot.writeFieldBegin(SINK_FIELD_DESC);
        oprot.writeString(this.sink);
        oprot.writeFieldEnd();
      }
      oprot.writeFieldStop();
      oprot.writeStructEnd();
    }

    private static class _CompatibleWithKeywordSinkLazy {
        private static final CompatibleWithKeywordSink _DEFAULT = new CompatibleWithKeywordSink.Builder().build();
    }
    
    public static CompatibleWithKeywordSink defaultInstance() {
        return  _CompatibleWithKeywordSinkLazy._DEFAULT;
    }
}
