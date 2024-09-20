// Autogenerated by Thrift for thrift/compiler/test/fixtures/inject_metadata_fields/src/module.thrift
//
// DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
//  @generated

package module

import (
    "fmt"
    "strings"

    foo "foo"
    thrift "github.com/facebook/fbthrift/thrift/lib/go/thrift/types"
)

var _ = foo.GoUnusedProtection__
// (needed to ensure safety because of naive import list construction)
var _ = fmt.Printf
var _ = strings.Split
var _ = thrift.ZERO


type Fields struct {
    InjectedField string `thrift:"injected_field,100" json:"injected_field" db:"injected_field"`
}
// Compile time interface enforcer
var _ thrift.Struct = (*Fields)(nil)

func NewFields() *Fields {
    return (&Fields{}).
        SetInjectedFieldNonCompat("")
}

func (x *Fields) GetInjectedField() string {
    return x.InjectedField
}

func (x *Fields) SetInjectedFieldNonCompat(value string) *Fields {
    x.InjectedField = value
    return x
}

func (x *Fields) SetInjectedField(value string) *Fields {
    x.InjectedField = value
    return x
}

func (x *Fields) writeField100(p thrift.Encoder) error {  // InjectedField
    if err := p.WriteFieldBegin("injected_field", thrift.STRING, 100); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write field begin error: ", x), err)
    }

    item := x.InjectedField
    if err := p.WriteString(item); err != nil {
    return err
}

    if err := p.WriteFieldEnd(); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write field end error: ", x), err)
    }
    return nil
}

func (x *Fields) readField100(p thrift.Decoder) error {  // InjectedField
    result, err := p.ReadString()
if err != nil {
    return err
}

    x.InjectedField = result
    return nil
}

func (x *Fields) toString100() string {  // InjectedField
    return fmt.Sprintf("%v", x.InjectedField)
}



func (x *Fields) Write(p thrift.Encoder) error {
    if err := p.WriteStructBegin("Fields"); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write struct begin error: ", x), err)
    }

    if err := x.writeField100(p); err != nil {
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

func (x *Fields) Read(p thrift.Decoder) error {
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

        switch {
        case (id == 100 && wireType == thrift.Type(thrift.STRING)):  // injected_field
            if err := x.readField100(p); err != nil {
                return err
            }
        default:
            if err := p.Skip(wireType); err != nil {
                return err
            }
        }

        if err := p.ReadFieldEnd(); err != nil {
            return err
        }
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
    sb.WriteString(fmt.Sprintf("InjectedField:%s", x.toString100()))
    sb.WriteString("})")

    return sb.String()
}

type FieldsInjectedToEmptyStruct struct {
    InjectedField string `thrift:"injected_field,-1100" json:"injected_field" db:"injected_field"`
}
// Compile time interface enforcer
var _ thrift.Struct = (*FieldsInjectedToEmptyStruct)(nil)

func NewFieldsInjectedToEmptyStruct() *FieldsInjectedToEmptyStruct {
    return (&FieldsInjectedToEmptyStruct{}).
        SetInjectedFieldNonCompat("")
}

func (x *FieldsInjectedToEmptyStruct) GetInjectedField() string {
    return x.InjectedField
}

func (x *FieldsInjectedToEmptyStruct) SetInjectedFieldNonCompat(value string) *FieldsInjectedToEmptyStruct {
    x.InjectedField = value
    return x
}

func (x *FieldsInjectedToEmptyStruct) SetInjectedField(value string) *FieldsInjectedToEmptyStruct {
    x.InjectedField = value
    return x
}

func (x *FieldsInjectedToEmptyStruct) writeField_1100(p thrift.Encoder) error {  // InjectedField
    if err := p.WriteFieldBegin("injected_field", thrift.STRING, -1100); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write field begin error: ", x), err)
    }

    item := x.InjectedField
    if err := p.WriteString(item); err != nil {
    return err
}

    if err := p.WriteFieldEnd(); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write field end error: ", x), err)
    }
    return nil
}

func (x *FieldsInjectedToEmptyStruct) readField_1100(p thrift.Decoder) error {  // InjectedField
    result, err := p.ReadString()
if err != nil {
    return err
}

    x.InjectedField = result
    return nil
}

func (x *FieldsInjectedToEmptyStruct) toString_1100() string {  // InjectedField
    return fmt.Sprintf("%v", x.InjectedField)
}



func (x *FieldsInjectedToEmptyStruct) Write(p thrift.Encoder) error {
    if err := p.WriteStructBegin("FieldsInjectedToEmptyStruct"); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write struct begin error: ", x), err)
    }

    if err := x.writeField_1100(p); err != nil {
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

func (x *FieldsInjectedToEmptyStruct) Read(p thrift.Decoder) error {
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

        switch {
        case (id == -1100 && wireType == thrift.Type(thrift.STRING)):  // injected_field
            if err := x.readField_1100(p); err != nil {
                return err
            }
        default:
            if err := p.Skip(wireType); err != nil {
                return err
            }
        }

        if err := p.ReadFieldEnd(); err != nil {
            return err
        }
    }

    if err := p.ReadStructEnd(); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T read struct end error: ", x), err)
    }

    return nil
}

func (x *FieldsInjectedToEmptyStruct) String() string {
    if x == nil {
        return "<nil>"
    }

    var sb strings.Builder

    sb.WriteString("FieldsInjectedToEmptyStruct({")
    sb.WriteString(fmt.Sprintf("InjectedField:%s", x.toString_1100()))
    sb.WriteString("})")

    return sb.String()
}

type FieldsInjectedToStruct struct {
    InjectedField string `thrift:"injected_field,-1100" json:"injected_field" db:"injected_field"`
    StringField string `thrift:"string_field,1" json:"string_field" db:"string_field"`
}
// Compile time interface enforcer
var _ thrift.Struct = (*FieldsInjectedToStruct)(nil)

func NewFieldsInjectedToStruct() *FieldsInjectedToStruct {
    return (&FieldsInjectedToStruct{}).
        SetInjectedFieldNonCompat("").
        SetStringFieldNonCompat("")
}

func (x *FieldsInjectedToStruct) GetInjectedField() string {
    return x.InjectedField
}

func (x *FieldsInjectedToStruct) GetStringField() string {
    return x.StringField
}

func (x *FieldsInjectedToStruct) SetInjectedFieldNonCompat(value string) *FieldsInjectedToStruct {
    x.InjectedField = value
    return x
}

func (x *FieldsInjectedToStruct) SetInjectedField(value string) *FieldsInjectedToStruct {
    x.InjectedField = value
    return x
}

func (x *FieldsInjectedToStruct) SetStringFieldNonCompat(value string) *FieldsInjectedToStruct {
    x.StringField = value
    return x
}

func (x *FieldsInjectedToStruct) SetStringField(value string) *FieldsInjectedToStruct {
    x.StringField = value
    return x
}

func (x *FieldsInjectedToStruct) writeField_1100(p thrift.Encoder) error {  // InjectedField
    if err := p.WriteFieldBegin("injected_field", thrift.STRING, -1100); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write field begin error: ", x), err)
    }

    item := x.InjectedField
    if err := p.WriteString(item); err != nil {
    return err
}

    if err := p.WriteFieldEnd(); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write field end error: ", x), err)
    }
    return nil
}

func (x *FieldsInjectedToStruct) writeField1(p thrift.Encoder) error {  // StringField
    if err := p.WriteFieldBegin("string_field", thrift.STRING, 1); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write field begin error: ", x), err)
    }

    item := x.StringField
    if err := p.WriteString(item); err != nil {
    return err
}

    if err := p.WriteFieldEnd(); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write field end error: ", x), err)
    }
    return nil
}

func (x *FieldsInjectedToStruct) readField_1100(p thrift.Decoder) error {  // InjectedField
    result, err := p.ReadString()
if err != nil {
    return err
}

    x.InjectedField = result
    return nil
}

func (x *FieldsInjectedToStruct) readField1(p thrift.Decoder) error {  // StringField
    result, err := p.ReadString()
if err != nil {
    return err
}

    x.StringField = result
    return nil
}

func (x *FieldsInjectedToStruct) toString_1100() string {  // InjectedField
    return fmt.Sprintf("%v", x.InjectedField)
}

func (x *FieldsInjectedToStruct) toString1() string {  // StringField
    return fmt.Sprintf("%v", x.StringField)
}



func (x *FieldsInjectedToStruct) Write(p thrift.Encoder) error {
    if err := p.WriteStructBegin("FieldsInjectedToStruct"); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write struct begin error: ", x), err)
    }

    if err := x.writeField_1100(p); err != nil {
        return err
    }

    if err := x.writeField1(p); err != nil {
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

func (x *FieldsInjectedToStruct) Read(p thrift.Decoder) error {
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

        switch {
        case (id == -1100 && wireType == thrift.Type(thrift.STRING)):  // injected_field
            if err := x.readField_1100(p); err != nil {
                return err
            }
        case (id == 1 && wireType == thrift.Type(thrift.STRING)):  // string_field
            if err := x.readField1(p); err != nil {
                return err
            }
        default:
            if err := p.Skip(wireType); err != nil {
                return err
            }
        }

        if err := p.ReadFieldEnd(); err != nil {
            return err
        }
    }

    if err := p.ReadStructEnd(); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T read struct end error: ", x), err)
    }

    return nil
}

func (x *FieldsInjectedToStruct) String() string {
    if x == nil {
        return "<nil>"
    }

    var sb strings.Builder

    sb.WriteString("FieldsInjectedToStruct({")
    sb.WriteString(fmt.Sprintf("InjectedField:%s ", x.toString_1100()))
    sb.WriteString(fmt.Sprintf("StringField:%s", x.toString1()))
    sb.WriteString("})")

    return sb.String()
}

type FieldsInjectedWithIncludedStruct struct {
    InjectedUnstructuredAnnotationField *string `thrift:"injected_unstructured_annotation_field,-1102,optional" json:"injected_unstructured_annotation_field,omitempty" db:"injected_unstructured_annotation_field"`
    InjectedStructuredAnnotationField *string `thrift:"injected_structured_annotation_field,-1101,optional" json:"injected_structured_annotation_field,omitempty" db:"injected_structured_annotation_field"`
    InjectedField string `thrift:"injected_field,-1100" json:"injected_field" db:"injected_field"`
    StringField string `thrift:"string_field,1" json:"string_field" db:"string_field"`
}
// Compile time interface enforcer
var _ thrift.Struct = (*FieldsInjectedWithIncludedStruct)(nil)

func NewFieldsInjectedWithIncludedStruct() *FieldsInjectedWithIncludedStruct {
    return (&FieldsInjectedWithIncludedStruct{}).
        SetInjectedFieldNonCompat("").
        SetStringFieldNonCompat("")
}

func (x *FieldsInjectedWithIncludedStruct) GetInjectedUnstructuredAnnotationField() string {
    if !x.IsSetInjectedUnstructuredAnnotationField() {
        return ""
    }

    return *x.InjectedUnstructuredAnnotationField
}

func (x *FieldsInjectedWithIncludedStruct) GetInjectedStructuredAnnotationField() string {
    if !x.IsSetInjectedStructuredAnnotationField() {
        return ""
    }

    return *x.InjectedStructuredAnnotationField
}

func (x *FieldsInjectedWithIncludedStruct) GetInjectedField() string {
    return x.InjectedField
}

func (x *FieldsInjectedWithIncludedStruct) GetStringField() string {
    return x.StringField
}

func (x *FieldsInjectedWithIncludedStruct) SetInjectedUnstructuredAnnotationFieldNonCompat(value string) *FieldsInjectedWithIncludedStruct {
    x.InjectedUnstructuredAnnotationField = &value
    return x
}

func (x *FieldsInjectedWithIncludedStruct) SetInjectedUnstructuredAnnotationField(value *string) *FieldsInjectedWithIncludedStruct {
    x.InjectedUnstructuredAnnotationField = value
    return x
}

func (x *FieldsInjectedWithIncludedStruct) SetInjectedStructuredAnnotationFieldNonCompat(value string) *FieldsInjectedWithIncludedStruct {
    x.InjectedStructuredAnnotationField = &value
    return x
}

func (x *FieldsInjectedWithIncludedStruct) SetInjectedStructuredAnnotationField(value *string) *FieldsInjectedWithIncludedStruct {
    x.InjectedStructuredAnnotationField = value
    return x
}

func (x *FieldsInjectedWithIncludedStruct) SetInjectedFieldNonCompat(value string) *FieldsInjectedWithIncludedStruct {
    x.InjectedField = value
    return x
}

func (x *FieldsInjectedWithIncludedStruct) SetInjectedField(value string) *FieldsInjectedWithIncludedStruct {
    x.InjectedField = value
    return x
}

func (x *FieldsInjectedWithIncludedStruct) SetStringFieldNonCompat(value string) *FieldsInjectedWithIncludedStruct {
    x.StringField = value
    return x
}

func (x *FieldsInjectedWithIncludedStruct) SetStringField(value string) *FieldsInjectedWithIncludedStruct {
    x.StringField = value
    return x
}

func (x *FieldsInjectedWithIncludedStruct) IsSetInjectedUnstructuredAnnotationField() bool {
    return x != nil && x.InjectedUnstructuredAnnotationField != nil
}

func (x *FieldsInjectedWithIncludedStruct) IsSetInjectedStructuredAnnotationField() bool {
    return x != nil && x.InjectedStructuredAnnotationField != nil
}

func (x *FieldsInjectedWithIncludedStruct) writeField_1102(p thrift.Encoder) error {  // InjectedUnstructuredAnnotationField
    if !x.IsSetInjectedUnstructuredAnnotationField() {
        return nil
    }

    if err := p.WriteFieldBegin("injected_unstructured_annotation_field", thrift.STRING, -1102); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write field begin error: ", x), err)
    }

    item := *x.InjectedUnstructuredAnnotationField
    if err := p.WriteString(item); err != nil {
    return err
}

    if err := p.WriteFieldEnd(); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write field end error: ", x), err)
    }
    return nil
}

func (x *FieldsInjectedWithIncludedStruct) writeField_1101(p thrift.Encoder) error {  // InjectedStructuredAnnotationField
    if !x.IsSetInjectedStructuredAnnotationField() {
        return nil
    }

    if err := p.WriteFieldBegin("injected_structured_annotation_field", thrift.STRING, -1101); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write field begin error: ", x), err)
    }

    item := *x.InjectedStructuredAnnotationField
    if err := p.WriteString(item); err != nil {
    return err
}

    if err := p.WriteFieldEnd(); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write field end error: ", x), err)
    }
    return nil
}

func (x *FieldsInjectedWithIncludedStruct) writeField_1100(p thrift.Encoder) error {  // InjectedField
    if err := p.WriteFieldBegin("injected_field", thrift.STRING, -1100); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write field begin error: ", x), err)
    }

    item := x.InjectedField
    if err := p.WriteString(item); err != nil {
    return err
}

    if err := p.WriteFieldEnd(); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write field end error: ", x), err)
    }
    return nil
}

func (x *FieldsInjectedWithIncludedStruct) writeField1(p thrift.Encoder) error {  // StringField
    if err := p.WriteFieldBegin("string_field", thrift.STRING, 1); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write field begin error: ", x), err)
    }

    item := x.StringField
    if err := p.WriteString(item); err != nil {
    return err
}

    if err := p.WriteFieldEnd(); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write field end error: ", x), err)
    }
    return nil
}

func (x *FieldsInjectedWithIncludedStruct) readField_1102(p thrift.Decoder) error {  // InjectedUnstructuredAnnotationField
    result, err := p.ReadString()
if err != nil {
    return err
}

    x.InjectedUnstructuredAnnotationField = &result
    return nil
}

func (x *FieldsInjectedWithIncludedStruct) readField_1101(p thrift.Decoder) error {  // InjectedStructuredAnnotationField
    result, err := p.ReadString()
if err != nil {
    return err
}

    x.InjectedStructuredAnnotationField = &result
    return nil
}

func (x *FieldsInjectedWithIncludedStruct) readField_1100(p thrift.Decoder) error {  // InjectedField
    result, err := p.ReadString()
if err != nil {
    return err
}

    x.InjectedField = result
    return nil
}

func (x *FieldsInjectedWithIncludedStruct) readField1(p thrift.Decoder) error {  // StringField
    result, err := p.ReadString()
if err != nil {
    return err
}

    x.StringField = result
    return nil
}

func (x *FieldsInjectedWithIncludedStruct) toString_1102() string {  // InjectedUnstructuredAnnotationField
    if x.IsSetInjectedUnstructuredAnnotationField() {
        return fmt.Sprintf("%v", *x.InjectedUnstructuredAnnotationField)
    }
    return fmt.Sprintf("%v", x.InjectedUnstructuredAnnotationField)
}

func (x *FieldsInjectedWithIncludedStruct) toString_1101() string {  // InjectedStructuredAnnotationField
    if x.IsSetInjectedStructuredAnnotationField() {
        return fmt.Sprintf("%v", *x.InjectedStructuredAnnotationField)
    }
    return fmt.Sprintf("%v", x.InjectedStructuredAnnotationField)
}

func (x *FieldsInjectedWithIncludedStruct) toString_1100() string {  // InjectedField
    return fmt.Sprintf("%v", x.InjectedField)
}

func (x *FieldsInjectedWithIncludedStruct) toString1() string {  // StringField
    return fmt.Sprintf("%v", x.StringField)
}





func (x *FieldsInjectedWithIncludedStruct) Write(p thrift.Encoder) error {
    if err := p.WriteStructBegin("FieldsInjectedWithIncludedStruct"); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write struct begin error: ", x), err)
    }

    if err := x.writeField_1102(p); err != nil {
        return err
    }

    if err := x.writeField_1101(p); err != nil {
        return err
    }

    if err := x.writeField_1100(p); err != nil {
        return err
    }

    if err := x.writeField1(p); err != nil {
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

func (x *FieldsInjectedWithIncludedStruct) Read(p thrift.Decoder) error {
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

        switch {
        case (id == -1102 && wireType == thrift.Type(thrift.STRING)):  // injected_unstructured_annotation_field
            if err := x.readField_1102(p); err != nil {
                return err
            }
        case (id == -1101 && wireType == thrift.Type(thrift.STRING)):  // injected_structured_annotation_field
            if err := x.readField_1101(p); err != nil {
                return err
            }
        case (id == -1100 && wireType == thrift.Type(thrift.STRING)):  // injected_field
            if err := x.readField_1100(p); err != nil {
                return err
            }
        case (id == 1 && wireType == thrift.Type(thrift.STRING)):  // string_field
            if err := x.readField1(p); err != nil {
                return err
            }
        default:
            if err := p.Skip(wireType); err != nil {
                return err
            }
        }

        if err := p.ReadFieldEnd(); err != nil {
            return err
        }
    }

    if err := p.ReadStructEnd(); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T read struct end error: ", x), err)
    }

    return nil
}

func (x *FieldsInjectedWithIncludedStruct) String() string {
    if x == nil {
        return "<nil>"
    }

    var sb strings.Builder

    sb.WriteString("FieldsInjectedWithIncludedStruct({")
    sb.WriteString(fmt.Sprintf("InjectedUnstructuredAnnotationField:%s ", x.toString_1102()))
    sb.WriteString(fmt.Sprintf("InjectedStructuredAnnotationField:%s ", x.toString_1101()))
    sb.WriteString(fmt.Sprintf("InjectedField:%s ", x.toString_1100()))
    sb.WriteString(fmt.Sprintf("StringField:%s", x.toString1()))
    sb.WriteString("})")

    return sb.String()
}

// RegisterTypes registers types found in this file that have a thrift_uri with the passed in registry.
func RegisterTypes(registry interface {
  RegisterType(name string, initializer func() any)
}) {

}
