/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */

package test.fixtures.exceptions;

import com.facebook.swift.codec.*;
import com.facebook.swift.codec.ThriftField.Requiredness;
import com.facebook.swift.codec.ThriftField.Recursiveness;
import java.util.*;
import javax.annotation.Nullable;
import org.apache.thrift.*;
import org.apache.thrift.transport.*;
import org.apache.thrift.protocol.*;
import com.google.common.collect.*;

@SwiftGenerated
@com.facebook.swift.codec.ThriftStruct("Serious")
public final class Serious extends org.apache.thrift.TBaseException implements com.facebook.thrift.payload.ThriftSerializable {
    private static final long serialVersionUID = 1L;

    
    public static final Map<String, Integer> NAMES_TO_IDS = new HashMap();
    public static final Map<String, Integer> THRIFT_NAMES_TO_IDS = new HashMap();
    public static final Map<Integer, TField> FIELD_METADATA = new HashMap<>();

    private static final TStruct STRUCT_DESC = new TStruct("Serious");
    private final String sonnet;
    public static final int _SONNET = 1;
    private static final TField SONNET_FIELD_DESC = new TField("sonnet", TType.STRING, (short)1);

    static {
      NAMES_TO_IDS.put("sonnet", 1);
      THRIFT_NAMES_TO_IDS.put("sonnet", 1);
      FIELD_METADATA.put(1, SONNET_FIELD_DESC);
    }

    @ThriftConstructor
    public Serious(
        @com.facebook.swift.codec.ThriftField(value=1, name="sonnet", requiredness=Requiredness.OPTIONAL) final String sonnet
    ) {
        this.sonnet = sonnet;
    }
    
    @ThriftConstructor
    protected Serious() {
      this.sonnet = null;
    }

    public static class Builder {
        private String sonnet = null;
    
        @com.facebook.swift.codec.ThriftField(value=1, name="sonnet", requiredness=Requiredness.OPTIONAL)    public Builder setSonnet(String sonnet) {
            this.sonnet = sonnet;
            return this;
        }
    
        public String getSonnet() { return sonnet; }
    
        public Builder() { }
        public Builder(Serious other) {
            this.sonnet = other.sonnet;
        }
    
        @ThriftConstructor
        public Serious build() {
            Serious result = new Serious (
                this.sonnet
            );
            return result;
        }
    }

    
    @Nullable
    @com.facebook.swift.codec.ThriftField(value=1, name="sonnet", requiredness=Requiredness.OPTIONAL)
    public String getSonnet() { return sonnet; }
    
    @java.lang.Override
    public String getMessage() {
      return sonnet;
    }
    
    public static com.facebook.thrift.payload.Reader<Serious> asReader() {
      return Serious::read0;
    }
    
    public static Serious read0(TProtocol oprot) throws TException {
      TField __field;
      oprot.readStructBegin(Serious.NAMES_TO_IDS, Serious.THRIFT_NAMES_TO_IDS, Serious.FIELD_METADATA);
      Serious.Builder builder = new Serious.Builder();
      while (true) {
        __field = oprot.readFieldBegin();
        if (__field.type == TType.STOP) { break; }
        switch (__field.id) {
        case _SONNET:
          if (__field.type == TType.STRING) {
            String  sonnet = oprot.readString();
            builder.setSonnet(sonnet);
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
      if (sonnet != null) {
        oprot.writeFieldBegin(SONNET_FIELD_DESC);
        oprot.writeString(this.sonnet);
        oprot.writeFieldEnd();
      }
      oprot.writeFieldStop();
      oprot.writeStructEnd();
    }

    private static class _SeriousLazy {
        private static final Serious _DEFAULT = new Serious.Builder().build();
    }
    
    public static Serious defaultInstance() {
        return  _SeriousLazy._DEFAULT;
    }}
