// @generated by Thrift for [[[ program path ]]]
// This file is probably not the place you want to edit!

package foo // [[[ program thrift source path ]]]

import (
    "fmt"
    "strings"

    thrift "github.com/facebook/fbthrift/thrift/lib/go/thrift"
)


// (needed to ensure safety because of naive import list construction)
var _ = fmt.Printf
var _ = thrift.ZERO
var _ = strings.Split


type Fields struct {
    InjectedField string `thrift:"injected_field,100" json:"injected_field" db:"injected_field"`
    InjectedStructuredAnnotationField *string `thrift:"injected_structured_annotation_field,101,optional" json:"injected_structured_annotation_field,omitempty" db:"injected_structured_annotation_field"`
    InjectedUnstructuredAnnotationField *string `thrift:"injected_unstructured_annotation_field,102,optional" json:"injected_unstructured_annotation_field,omitempty" db:"injected_unstructured_annotation_field"`
}
// Compile time interface enforcer
var _ thrift.Struct = &Fields{}

func NewFields() *Fields {
    return (&Fields{}).
        SetInjectedFieldNonCompat("")
}

func (x *Fields) GetInjectedFieldNonCompat() string {
    return x.InjectedField
}

func (x *Fields) GetInjectedField() string {
    return x.InjectedField
}

func (x *Fields) GetInjectedStructuredAnnotationFieldNonCompat() *string {
    return x.InjectedStructuredAnnotationField
}

func (x *Fields) GetInjectedStructuredAnnotationField() string {
    if !x.IsSetInjectedStructuredAnnotationField() {
        return ""
    }

    return *x.InjectedStructuredAnnotationField
}

func (x *Fields) GetInjectedUnstructuredAnnotationFieldNonCompat() *string {
    return x.InjectedUnstructuredAnnotationField
}

func (x *Fields) GetInjectedUnstructuredAnnotationField() string {
    if !x.IsSetInjectedUnstructuredAnnotationField() {
        return ""
    }

    return *x.InjectedUnstructuredAnnotationField
}

func (x *Fields) SetInjectedFieldNonCompat(value string) *Fields {
    x.InjectedField = value
    return x
}

func (x *Fields) SetInjectedField(value string) *Fields {
    x.InjectedField = value
    return x
}

func (x *Fields) SetInjectedStructuredAnnotationFieldNonCompat(value string) *Fields {
    x.InjectedStructuredAnnotationField = &value
    return x
}

func (x *Fields) SetInjectedStructuredAnnotationField(value *string) *Fields {
    x.InjectedStructuredAnnotationField = value
    return x
}

func (x *Fields) SetInjectedUnstructuredAnnotationFieldNonCompat(value string) *Fields {
    x.InjectedUnstructuredAnnotationField = &value
    return x
}

func (x *Fields) SetInjectedUnstructuredAnnotationField(value *string) *Fields {
    x.InjectedUnstructuredAnnotationField = value
    return x
}

func (x *Fields) IsSetInjectedStructuredAnnotationField() bool {
    return x.InjectedStructuredAnnotationField != nil
}

func (x *Fields) IsSetInjectedUnstructuredAnnotationField() bool {
    return x.InjectedUnstructuredAnnotationField != nil
}

func (x *Fields) writeField100(p thrift.Protocol) error {  // InjectedField
    if err := p.WriteFieldBegin("injected_field", thrift.STRING, 100); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write field begin error: ", x), err)
    }

    item := x.GetInjectedFieldNonCompat()
    if err := p.WriteString(item); err != nil {
    return err
}

    if err := p.WriteFieldEnd(); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write field end error: ", x), err)
    }
    return nil
}

func (x *Fields) writeField101(p thrift.Protocol) error {  // InjectedStructuredAnnotationField
    if !x.IsSetInjectedStructuredAnnotationField() {
        return nil
    }

    if err := p.WriteFieldBegin("injected_structured_annotation_field", thrift.STRING, 101); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write field begin error: ", x), err)
    }

    item := *x.GetInjectedStructuredAnnotationFieldNonCompat()
    if err := p.WriteString(item); err != nil {
    return err
}

    if err := p.WriteFieldEnd(); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write field end error: ", x), err)
    }
    return nil
}

func (x *Fields) writeField102(p thrift.Protocol) error {  // InjectedUnstructuredAnnotationField
    if !x.IsSetInjectedUnstructuredAnnotationField() {
        return nil
    }

    if err := p.WriteFieldBegin("injected_unstructured_annotation_field", thrift.STRING, 102); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write field begin error: ", x), err)
    }

    item := *x.GetInjectedUnstructuredAnnotationFieldNonCompat()
    if err := p.WriteString(item); err != nil {
    return err
}

    if err := p.WriteFieldEnd(); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write field end error: ", x), err)
    }
    return nil
}

func (x *Fields) readField100(p thrift.Protocol) error {  // InjectedField
    result, err := p.ReadString()
if err != nil {
    return err
}

    x.SetInjectedFieldNonCompat(result)
    return nil
}

func (x *Fields) readField101(p thrift.Protocol) error {  // InjectedStructuredAnnotationField
    result, err := p.ReadString()
if err != nil {
    return err
}

    x.SetInjectedStructuredAnnotationFieldNonCompat(result)
    return nil
}

func (x *Fields) readField102(p thrift.Protocol) error {  // InjectedUnstructuredAnnotationField
    result, err := p.ReadString()
if err != nil {
    return err
}

    x.SetInjectedUnstructuredAnnotationFieldNonCompat(result)
    return nil
}

func (x *Fields) toString100() string {  // InjectedField
    return fmt.Sprintf("%v", x.GetInjectedFieldNonCompat())
}

func (x *Fields) toString101() string {  // InjectedStructuredAnnotationField
    if x.IsSetInjectedStructuredAnnotationField() {
        return fmt.Sprintf("%v", *x.GetInjectedStructuredAnnotationFieldNonCompat())
    }
    return fmt.Sprintf("%v", x.GetInjectedStructuredAnnotationFieldNonCompat())
}

func (x *Fields) toString102() string {  // InjectedUnstructuredAnnotationField
    if x.IsSetInjectedUnstructuredAnnotationField() {
        return fmt.Sprintf("%v", *x.GetInjectedUnstructuredAnnotationFieldNonCompat())
    }
    return fmt.Sprintf("%v", x.GetInjectedUnstructuredAnnotationFieldNonCompat())
}

// Deprecated: Use NewFields().GetInjectedStructuredAnnotationField() instead.
var Fields_InjectedStructuredAnnotationField_DEFAULT = NewFields().GetInjectedStructuredAnnotationField()

// Deprecated: Use NewFields().GetInjectedUnstructuredAnnotationField() instead.
var Fields_InjectedUnstructuredAnnotationField_DEFAULT = NewFields().GetInjectedUnstructuredAnnotationField()


// Deprecated: Use Fields.Set* methods instead or set the fields directly.
type FieldsBuilder struct {
    obj *Fields
}

func NewFieldsBuilder() *FieldsBuilder {
    return &FieldsBuilder{
        obj: NewFields(),
    }
}

func (x *FieldsBuilder) InjectedField(value string) *FieldsBuilder {
    x.obj.InjectedField = value
    return x
}

func (x *FieldsBuilder) InjectedStructuredAnnotationField(value *string) *FieldsBuilder {
    x.obj.InjectedStructuredAnnotationField = value
    return x
}

func (x *FieldsBuilder) InjectedUnstructuredAnnotationField(value *string) *FieldsBuilder {
    x.obj.InjectedUnstructuredAnnotationField = value
    return x
}

func (x *FieldsBuilder) Emit() *Fields {
    var objCopy Fields = *x.obj
    return &objCopy
}

func (x *Fields) Write(p thrift.Protocol) error {
    if err := p.WriteStructBegin("Fields"); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write struct begin error: ", x), err)
    }

    if err := x.writeField100(p); err != nil {
        return err
    }

    if err := x.writeField101(p); err != nil {
        return err
    }

    if err := x.writeField102(p); err != nil {
        return err
    }

    if err := p.WriteFieldStop(); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write field stop error: ", x), err)
    }

    if err := p.WriteStructEnd(); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write struct end error: ", x), err)
    }
    return nil
}

func (x *Fields) Read(p thrift.Protocol) error {
    if _, err := p.ReadStructBegin(); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T read error: ", x), err)
    }

    for {
        _, wireType, id, err := p.ReadFieldBegin()
        if err != nil {
            return thrift.PrependError(fmt.Sprintf("%T field %d read error: ", x, id), err)
        }

        if wireType == thrift.STOP {
            break;
        }

        switch id {
        case 100:  // injected_field
            expectedType := thrift.Type(thrift.STRING)
            if wireType == expectedType {
                if err := x.readField100(p); err != nil {
                   return err
                }
            } else {
                if err := p.Skip(wireType); err != nil {
                    return err
                }
            }
        case 101:  // injected_structured_annotation_field
            expectedType := thrift.Type(thrift.STRING)
            if wireType == expectedType {
                if err := x.readField101(p); err != nil {
                   return err
                }
            } else {
                if err := p.Skip(wireType); err != nil {
                    return err
                }
            }
        case 102:  // injected_unstructured_annotation_field
            expectedType := thrift.Type(thrift.STRING)
            if wireType == expectedType {
                if err := x.readField102(p); err != nil {
                   return err
                }
            } else {
                if err := p.Skip(wireType); err != nil {
                    return err
                }
            }
        default:
            if err := p.Skip(wireType); err != nil {
                return err
            }
        }
    }

    if err := p.ReadFieldEnd(); err != nil {
        return err
    }

    if err := p.ReadStructEnd(); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T read struct end error: ", x), err)
    }

    return nil
}

func (x *Fields) String() string {
    if x == nil {
        return "<nil>"
    }

    var sb strings.Builder

    sb.WriteString("Fields({")
    sb.WriteString(fmt.Sprintf("InjectedField:%s ", x.toString100()))
    sb.WriteString(fmt.Sprintf("InjectedStructuredAnnotationField:%s ", x.toString101()))
    sb.WriteString(fmt.Sprintf("InjectedUnstructuredAnnotationField:%s", x.toString102()))
    sb.WriteString("})")

    return sb.String()
}

// RegisterTypes registers types found in this file that have a thrift_uri with the passed in registry.
func RegisterTypes(registry interface {
	  RegisterType(name string, initializer func() any)
}) {

}
