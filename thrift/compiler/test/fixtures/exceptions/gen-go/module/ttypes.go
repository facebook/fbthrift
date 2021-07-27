// Autogenerated by Thrift Compiler (facebook)
// DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
// @generated

package module

import (
	"bytes"
	"context"
	"sync"
	"fmt"
	thrift "github.com/facebook/fbthrift/thrift/lib/go/thrift"
)

// (needed to ensure safety because of naive import list construction.)
var _ = thrift.ZERO
var _ = fmt.Printf
var _ = sync.Mutex{}
var _ = bytes.Equal
var _ = context.Background

var GoUnusedProtection__ int;

// Attributes:
//  - Message
type Fiery struct {
  Message string `thrift:"message,1,required" db:"message" json:"message"`
}

func NewFiery() *Fiery {
  return &Fiery{}
}


func (p *Fiery) GetMessage() string {
  return p.Message
}
type FieryBuilder struct {
  obj *Fiery
}

func NewFieryBuilder() *FieryBuilder{
  return &FieryBuilder{
    obj: NewFiery(),
  }
}

func (p FieryBuilder) Emit() *Fiery{
  return &Fiery{
    Message: p.obj.Message,
  }
}

func (f *FieryBuilder) Message(message string) *FieryBuilder {
  f.obj.Message = message
  return f
}

func (f *Fiery) SetMessage(message string) *Fiery {
  f.Message = message
  return f
}

func (p *Fiery) Read(iprot thrift.Protocol) error {
  if _, err := iprot.ReadStructBegin(); err != nil {
    return thrift.PrependError(fmt.Sprintf("%T read error: ", p), err)
  }

  var issetMessage bool = false;

  for {
    _, fieldTypeId, fieldId, err := iprot.ReadFieldBegin()
    if err != nil {
      return thrift.PrependError(fmt.Sprintf("%T field %d read error: ", p, fieldId), err)
    }
    if fieldTypeId == thrift.STOP { break; }
    switch fieldId {
    case 1:
      if err := p.ReadField1(iprot); err != nil {
        return err
      }
      issetMessage = true
    default:
      if err := iprot.Skip(fieldTypeId); err != nil {
        return err
      }
    }
    if err := iprot.ReadFieldEnd(); err != nil {
      return err
    }
  }
  if err := iprot.ReadStructEnd(); err != nil {
    return thrift.PrependError(fmt.Sprintf("%T read struct end error: ", p), err)
  }
  if !issetMessage{
    return thrift.NewProtocolExceptionWithType(thrift.INVALID_DATA, fmt.Errorf("Required field Message is not set"));
  }
  return nil
}

func (p *Fiery)  ReadField1(iprot thrift.Protocol) error {
  if v, err := iprot.ReadString(); err != nil {
    return thrift.PrependError("error reading field 1: ", err)
  } else {
    p.Message = v
  }
  return nil
}

func (p *Fiery) Write(oprot thrift.Protocol) error {
  if err := oprot.WriteStructBegin("Fiery"); err != nil {
    return thrift.PrependError(fmt.Sprintf("%T write struct begin error: ", p), err) }
  if err := p.writeField1(oprot); err != nil { return err }
  if err := oprot.WriteFieldStop(); err != nil {
    return thrift.PrependError("write field stop error: ", err) }
  if err := oprot.WriteStructEnd(); err != nil {
    return thrift.PrependError("write struct stop error: ", err) }
  return nil
}

func (p *Fiery) writeField1(oprot thrift.Protocol) (err error) {
  if err := oprot.WriteFieldBegin("message", thrift.STRING, 1); err != nil {
    return thrift.PrependError(fmt.Sprintf("%T write field begin error 1:message: ", p), err) }
  if err := oprot.WriteString(string(p.Message)); err != nil {
  return thrift.PrependError(fmt.Sprintf("%T.message (1) field write error: ", p), err) }
  if err := oprot.WriteFieldEnd(); err != nil {
    return thrift.PrependError(fmt.Sprintf("%T write field end error 1:message: ", p), err) }
  return err
}

func (p *Fiery) String() string {
  if p == nil {
    return "<nil>"
  }

  messageVal := fmt.Sprintf("%v", p.Message)
  return fmt.Sprintf("Fiery({Message:%s})", messageVal)
}

func (p *Fiery) Error() string {
  return p.String()
}

// Attributes:
//  - Sonnet
type Serious struct {
  Sonnet *string `thrift:"sonnet,1,optional" db:"sonnet" json:"sonnet,omitempty"`
}

func NewSerious() *Serious {
  return &Serious{}
}

var Serious_Sonnet_DEFAULT string
func (p *Serious) GetSonnet() string {
  if !p.IsSetSonnet() {
    return Serious_Sonnet_DEFAULT
  }
return *p.Sonnet
}
func (p *Serious) IsSetSonnet() bool {
  return p != nil && p.Sonnet != nil
}

type SeriousBuilder struct {
  obj *Serious
}

func NewSeriousBuilder() *SeriousBuilder{
  return &SeriousBuilder{
    obj: NewSerious(),
  }
}

func (p SeriousBuilder) Emit() *Serious{
  return &Serious{
    Sonnet: p.obj.Sonnet,
  }
}

func (s *SeriousBuilder) Sonnet(sonnet *string) *SeriousBuilder {
  s.obj.Sonnet = sonnet
  return s
}

func (s *Serious) SetSonnet(sonnet *string) *Serious {
  s.Sonnet = sonnet
  return s
}

func (p *Serious) Read(iprot thrift.Protocol) error {
  if _, err := iprot.ReadStructBegin(); err != nil {
    return thrift.PrependError(fmt.Sprintf("%T read error: ", p), err)
  }


  for {
    _, fieldTypeId, fieldId, err := iprot.ReadFieldBegin()
    if err != nil {
      return thrift.PrependError(fmt.Sprintf("%T field %d read error: ", p, fieldId), err)
    }
    if fieldTypeId == thrift.STOP { break; }
    switch fieldId {
    case 1:
      if err := p.ReadField1(iprot); err != nil {
        return err
      }
    default:
      if err := iprot.Skip(fieldTypeId); err != nil {
        return err
      }
    }
    if err := iprot.ReadFieldEnd(); err != nil {
      return err
    }
  }
  if err := iprot.ReadStructEnd(); err != nil {
    return thrift.PrependError(fmt.Sprintf("%T read struct end error: ", p), err)
  }
  return nil
}

func (p *Serious)  ReadField1(iprot thrift.Protocol) error {
  if v, err := iprot.ReadString(); err != nil {
    return thrift.PrependError("error reading field 1: ", err)
  } else {
    p.Sonnet = &v
  }
  return nil
}

func (p *Serious) Write(oprot thrift.Protocol) error {
  if err := oprot.WriteStructBegin("Serious"); err != nil {
    return thrift.PrependError(fmt.Sprintf("%T write struct begin error: ", p), err) }
  if err := p.writeField1(oprot); err != nil { return err }
  if err := oprot.WriteFieldStop(); err != nil {
    return thrift.PrependError("write field stop error: ", err) }
  if err := oprot.WriteStructEnd(); err != nil {
    return thrift.PrependError("write struct stop error: ", err) }
  return nil
}

func (p *Serious) writeField1(oprot thrift.Protocol) (err error) {
  if p.IsSetSonnet() {
    if err := oprot.WriteFieldBegin("sonnet", thrift.STRING, 1); err != nil {
      return thrift.PrependError(fmt.Sprintf("%T write field begin error 1:sonnet: ", p), err) }
    if err := oprot.WriteString(string(*p.Sonnet)); err != nil {
    return thrift.PrependError(fmt.Sprintf("%T.sonnet (1) field write error: ", p), err) }
    if err := oprot.WriteFieldEnd(); err != nil {
      return thrift.PrependError(fmt.Sprintf("%T write field end error 1:sonnet: ", p), err) }
  }
  return err
}

func (p *Serious) String() string {
  if p == nil {
    return "<nil>"
  }

  var sonnetVal string
  if p.Sonnet == nil {
    sonnetVal = "<nil>"
  } else {
    sonnetVal = fmt.Sprintf("%v", *p.Sonnet)
  }
  return fmt.Sprintf("Serious({Sonnet:%s})", sonnetVal)
}

func (p *Serious) Error() string {
  return p.String()
}

// Attributes:
//  - ErrorMessage
//  - InternalErrorMessage
type ComplexFieldNames struct {
  ErrorMessage string `thrift:"error_message,1" db:"error_message" json:"error_message"`
  InternalErrorMessage string `thrift:"internal_error_message,2" db:"internal_error_message" json:"internal_error_message"`
}

func NewComplexFieldNames() *ComplexFieldNames {
  return &ComplexFieldNames{}
}


func (p *ComplexFieldNames) GetErrorMessage() string {
  return p.ErrorMessage
}

func (p *ComplexFieldNames) GetInternalErrorMessage() string {
  return p.InternalErrorMessage
}
type ComplexFieldNamesBuilder struct {
  obj *ComplexFieldNames
}

func NewComplexFieldNamesBuilder() *ComplexFieldNamesBuilder{
  return &ComplexFieldNamesBuilder{
    obj: NewComplexFieldNames(),
  }
}

func (p ComplexFieldNamesBuilder) Emit() *ComplexFieldNames{
  return &ComplexFieldNames{
    ErrorMessage: p.obj.ErrorMessage,
    InternalErrorMessage: p.obj.InternalErrorMessage,
  }
}

func (c *ComplexFieldNamesBuilder) ErrorMessage(errorMessage string) *ComplexFieldNamesBuilder {
  c.obj.ErrorMessage = errorMessage
  return c
}

func (c *ComplexFieldNamesBuilder) InternalErrorMessage(internalErrorMessage string) *ComplexFieldNamesBuilder {
  c.obj.InternalErrorMessage = internalErrorMessage
  return c
}

func (c *ComplexFieldNames) SetErrorMessage(errorMessage string) *ComplexFieldNames {
  c.ErrorMessage = errorMessage
  return c
}

func (c *ComplexFieldNames) SetInternalErrorMessage(internalErrorMessage string) *ComplexFieldNames {
  c.InternalErrorMessage = internalErrorMessage
  return c
}

func (p *ComplexFieldNames) Read(iprot thrift.Protocol) error {
  if _, err := iprot.ReadStructBegin(); err != nil {
    return thrift.PrependError(fmt.Sprintf("%T read error: ", p), err)
  }


  for {
    _, fieldTypeId, fieldId, err := iprot.ReadFieldBegin()
    if err != nil {
      return thrift.PrependError(fmt.Sprintf("%T field %d read error: ", p, fieldId), err)
    }
    if fieldTypeId == thrift.STOP { break; }
    switch fieldId {
    case 1:
      if err := p.ReadField1(iprot); err != nil {
        return err
      }
    case 2:
      if err := p.ReadField2(iprot); err != nil {
        return err
      }
    default:
      if err := iprot.Skip(fieldTypeId); err != nil {
        return err
      }
    }
    if err := iprot.ReadFieldEnd(); err != nil {
      return err
    }
  }
  if err := iprot.ReadStructEnd(); err != nil {
    return thrift.PrependError(fmt.Sprintf("%T read struct end error: ", p), err)
  }
  return nil
}

func (p *ComplexFieldNames)  ReadField1(iprot thrift.Protocol) error {
  if v, err := iprot.ReadString(); err != nil {
    return thrift.PrependError("error reading field 1: ", err)
  } else {
    p.ErrorMessage = v
  }
  return nil
}

func (p *ComplexFieldNames)  ReadField2(iprot thrift.Protocol) error {
  if v, err := iprot.ReadString(); err != nil {
    return thrift.PrependError("error reading field 2: ", err)
  } else {
    p.InternalErrorMessage = v
  }
  return nil
}

func (p *ComplexFieldNames) Write(oprot thrift.Protocol) error {
  if err := oprot.WriteStructBegin("ComplexFieldNames"); err != nil {
    return thrift.PrependError(fmt.Sprintf("%T write struct begin error: ", p), err) }
  if err := p.writeField1(oprot); err != nil { return err }
  if err := p.writeField2(oprot); err != nil { return err }
  if err := oprot.WriteFieldStop(); err != nil {
    return thrift.PrependError("write field stop error: ", err) }
  if err := oprot.WriteStructEnd(); err != nil {
    return thrift.PrependError("write struct stop error: ", err) }
  return nil
}

func (p *ComplexFieldNames) writeField1(oprot thrift.Protocol) (err error) {
  if err := oprot.WriteFieldBegin("error_message", thrift.STRING, 1); err != nil {
    return thrift.PrependError(fmt.Sprintf("%T write field begin error 1:error_message: ", p), err) }
  if err := oprot.WriteString(string(p.ErrorMessage)); err != nil {
  return thrift.PrependError(fmt.Sprintf("%T.error_message (1) field write error: ", p), err) }
  if err := oprot.WriteFieldEnd(); err != nil {
    return thrift.PrependError(fmt.Sprintf("%T write field end error 1:error_message: ", p), err) }
  return err
}

func (p *ComplexFieldNames) writeField2(oprot thrift.Protocol) (err error) {
  if err := oprot.WriteFieldBegin("internal_error_message", thrift.STRING, 2); err != nil {
    return thrift.PrependError(fmt.Sprintf("%T write field begin error 2:internal_error_message: ", p), err) }
  if err := oprot.WriteString(string(p.InternalErrorMessage)); err != nil {
  return thrift.PrependError(fmt.Sprintf("%T.internal_error_message (2) field write error: ", p), err) }
  if err := oprot.WriteFieldEnd(); err != nil {
    return thrift.PrependError(fmt.Sprintf("%T write field end error 2:internal_error_message: ", p), err) }
  return err
}

func (p *ComplexFieldNames) String() string {
  if p == nil {
    return "<nil>"
  }

  errorMessageVal := fmt.Sprintf("%v", p.ErrorMessage)
  internalErrorMessageVal := fmt.Sprintf("%v", p.InternalErrorMessage)
  return fmt.Sprintf("ComplexFieldNames({ErrorMessage:%s InternalErrorMessage:%s})", errorMessageVal, internalErrorMessageVal)
}

func (p *ComplexFieldNames) Error() string {
  return p.String()
}

// Attributes:
//  - ErrorMessage
//  - InternalErrorMessage
type CustomFieldNames struct {
  ErrorMessage string `thrift:"error_message,1" db:"error_message" json:"error_message"`
  InternalErrorMessage string `thrift:"internal_error_message,2" db:"internal_error_message" json:"internal_error_message"`
}

func NewCustomFieldNames() *CustomFieldNames {
  return &CustomFieldNames{}
}


func (p *CustomFieldNames) GetErrorMessage() string {
  return p.ErrorMessage
}

func (p *CustomFieldNames) GetInternalErrorMessage() string {
  return p.InternalErrorMessage
}
type CustomFieldNamesBuilder struct {
  obj *CustomFieldNames
}

func NewCustomFieldNamesBuilder() *CustomFieldNamesBuilder{
  return &CustomFieldNamesBuilder{
    obj: NewCustomFieldNames(),
  }
}

func (p CustomFieldNamesBuilder) Emit() *CustomFieldNames{
  return &CustomFieldNames{
    ErrorMessage: p.obj.ErrorMessage,
    InternalErrorMessage: p.obj.InternalErrorMessage,
  }
}

func (c *CustomFieldNamesBuilder) ErrorMessage(errorMessage string) *CustomFieldNamesBuilder {
  c.obj.ErrorMessage = errorMessage
  return c
}

func (c *CustomFieldNamesBuilder) InternalErrorMessage(internalErrorMessage string) *CustomFieldNamesBuilder {
  c.obj.InternalErrorMessage = internalErrorMessage
  return c
}

func (c *CustomFieldNames) SetErrorMessage(errorMessage string) *CustomFieldNames {
  c.ErrorMessage = errorMessage
  return c
}

func (c *CustomFieldNames) SetInternalErrorMessage(internalErrorMessage string) *CustomFieldNames {
  c.InternalErrorMessage = internalErrorMessage
  return c
}

func (p *CustomFieldNames) Read(iprot thrift.Protocol) error {
  if _, err := iprot.ReadStructBegin(); err != nil {
    return thrift.PrependError(fmt.Sprintf("%T read error: ", p), err)
  }


  for {
    _, fieldTypeId, fieldId, err := iprot.ReadFieldBegin()
    if err != nil {
      return thrift.PrependError(fmt.Sprintf("%T field %d read error: ", p, fieldId), err)
    }
    if fieldTypeId == thrift.STOP { break; }
    switch fieldId {
    case 1:
      if err := p.ReadField1(iprot); err != nil {
        return err
      }
    case 2:
      if err := p.ReadField2(iprot); err != nil {
        return err
      }
    default:
      if err := iprot.Skip(fieldTypeId); err != nil {
        return err
      }
    }
    if err := iprot.ReadFieldEnd(); err != nil {
      return err
    }
  }
  if err := iprot.ReadStructEnd(); err != nil {
    return thrift.PrependError(fmt.Sprintf("%T read struct end error: ", p), err)
  }
  return nil
}

func (p *CustomFieldNames)  ReadField1(iprot thrift.Protocol) error {
  if v, err := iprot.ReadString(); err != nil {
    return thrift.PrependError("error reading field 1: ", err)
  } else {
    p.ErrorMessage = v
  }
  return nil
}

func (p *CustomFieldNames)  ReadField2(iprot thrift.Protocol) error {
  if v, err := iprot.ReadString(); err != nil {
    return thrift.PrependError("error reading field 2: ", err)
  } else {
    p.InternalErrorMessage = v
  }
  return nil
}

func (p *CustomFieldNames) Write(oprot thrift.Protocol) error {
  if err := oprot.WriteStructBegin("CustomFieldNames"); err != nil {
    return thrift.PrependError(fmt.Sprintf("%T write struct begin error: ", p), err) }
  if err := p.writeField1(oprot); err != nil { return err }
  if err := p.writeField2(oprot); err != nil { return err }
  if err := oprot.WriteFieldStop(); err != nil {
    return thrift.PrependError("write field stop error: ", err) }
  if err := oprot.WriteStructEnd(); err != nil {
    return thrift.PrependError("write struct stop error: ", err) }
  return nil
}

func (p *CustomFieldNames) writeField1(oprot thrift.Protocol) (err error) {
  if err := oprot.WriteFieldBegin("error_message", thrift.STRING, 1); err != nil {
    return thrift.PrependError(fmt.Sprintf("%T write field begin error 1:error_message: ", p), err) }
  if err := oprot.WriteString(string(p.ErrorMessage)); err != nil {
  return thrift.PrependError(fmt.Sprintf("%T.error_message (1) field write error: ", p), err) }
  if err := oprot.WriteFieldEnd(); err != nil {
    return thrift.PrependError(fmt.Sprintf("%T write field end error 1:error_message: ", p), err) }
  return err
}

func (p *CustomFieldNames) writeField2(oprot thrift.Protocol) (err error) {
  if err := oprot.WriteFieldBegin("internal_error_message", thrift.STRING, 2); err != nil {
    return thrift.PrependError(fmt.Sprintf("%T write field begin error 2:internal_error_message: ", p), err) }
  if err := oprot.WriteString(string(p.InternalErrorMessage)); err != nil {
  return thrift.PrependError(fmt.Sprintf("%T.internal_error_message (2) field write error: ", p), err) }
  if err := oprot.WriteFieldEnd(); err != nil {
    return thrift.PrependError(fmt.Sprintf("%T write field end error 2:internal_error_message: ", p), err) }
  return err
}

func (p *CustomFieldNames) String() string {
  if p == nil {
    return "<nil>"
  }

  errorMessageVal := fmt.Sprintf("%v", p.ErrorMessage)
  internalErrorMessageVal := fmt.Sprintf("%v", p.InternalErrorMessage)
  return fmt.Sprintf("CustomFieldNames({ErrorMessage:%s InternalErrorMessage:%s})", errorMessageVal, internalErrorMessageVal)
}

func (p *CustomFieldNames) Error() string {
  return p.String()
}

// Attributes:
//  - Message
//  - ErrorCode
type ExceptionWithPrimitiveField struct {
  Message string `thrift:"message,1" db:"message" json:"message"`
  ErrorCode int32 `thrift:"error_code,2" db:"error_code" json:"error_code"`
}

func NewExceptionWithPrimitiveField() *ExceptionWithPrimitiveField {
  return &ExceptionWithPrimitiveField{}
}


func (p *ExceptionWithPrimitiveField) GetMessage() string {
  return p.Message
}

func (p *ExceptionWithPrimitiveField) GetErrorCode() int32 {
  return p.ErrorCode
}
type ExceptionWithPrimitiveFieldBuilder struct {
  obj *ExceptionWithPrimitiveField
}

func NewExceptionWithPrimitiveFieldBuilder() *ExceptionWithPrimitiveFieldBuilder{
  return &ExceptionWithPrimitiveFieldBuilder{
    obj: NewExceptionWithPrimitiveField(),
  }
}

func (p ExceptionWithPrimitiveFieldBuilder) Emit() *ExceptionWithPrimitiveField{
  return &ExceptionWithPrimitiveField{
    Message: p.obj.Message,
    ErrorCode: p.obj.ErrorCode,
  }
}

func (e *ExceptionWithPrimitiveFieldBuilder) Message(message string) *ExceptionWithPrimitiveFieldBuilder {
  e.obj.Message = message
  return e
}

func (e *ExceptionWithPrimitiveFieldBuilder) ErrorCode(errorCode int32) *ExceptionWithPrimitiveFieldBuilder {
  e.obj.ErrorCode = errorCode
  return e
}

func (e *ExceptionWithPrimitiveField) SetMessage(message string) *ExceptionWithPrimitiveField {
  e.Message = message
  return e
}

func (e *ExceptionWithPrimitiveField) SetErrorCode(errorCode int32) *ExceptionWithPrimitiveField {
  e.ErrorCode = errorCode
  return e
}

func (p *ExceptionWithPrimitiveField) Read(iprot thrift.Protocol) error {
  if _, err := iprot.ReadStructBegin(); err != nil {
    return thrift.PrependError(fmt.Sprintf("%T read error: ", p), err)
  }


  for {
    _, fieldTypeId, fieldId, err := iprot.ReadFieldBegin()
    if err != nil {
      return thrift.PrependError(fmt.Sprintf("%T field %d read error: ", p, fieldId), err)
    }
    if fieldTypeId == thrift.STOP { break; }
    switch fieldId {
    case 1:
      if err := p.ReadField1(iprot); err != nil {
        return err
      }
    case 2:
      if err := p.ReadField2(iprot); err != nil {
        return err
      }
    default:
      if err := iprot.Skip(fieldTypeId); err != nil {
        return err
      }
    }
    if err := iprot.ReadFieldEnd(); err != nil {
      return err
    }
  }
  if err := iprot.ReadStructEnd(); err != nil {
    return thrift.PrependError(fmt.Sprintf("%T read struct end error: ", p), err)
  }
  return nil
}

func (p *ExceptionWithPrimitiveField)  ReadField1(iprot thrift.Protocol) error {
  if v, err := iprot.ReadString(); err != nil {
    return thrift.PrependError("error reading field 1: ", err)
  } else {
    p.Message = v
  }
  return nil
}

func (p *ExceptionWithPrimitiveField)  ReadField2(iprot thrift.Protocol) error {
  if v, err := iprot.ReadI32(); err != nil {
    return thrift.PrependError("error reading field 2: ", err)
  } else {
    p.ErrorCode = v
  }
  return nil
}

func (p *ExceptionWithPrimitiveField) Write(oprot thrift.Protocol) error {
  if err := oprot.WriteStructBegin("ExceptionWithPrimitiveField"); err != nil {
    return thrift.PrependError(fmt.Sprintf("%T write struct begin error: ", p), err) }
  if err := p.writeField1(oprot); err != nil { return err }
  if err := p.writeField2(oprot); err != nil { return err }
  if err := oprot.WriteFieldStop(); err != nil {
    return thrift.PrependError("write field stop error: ", err) }
  if err := oprot.WriteStructEnd(); err != nil {
    return thrift.PrependError("write struct stop error: ", err) }
  return nil
}

func (p *ExceptionWithPrimitiveField) writeField1(oprot thrift.Protocol) (err error) {
  if err := oprot.WriteFieldBegin("message", thrift.STRING, 1); err != nil {
    return thrift.PrependError(fmt.Sprintf("%T write field begin error 1:message: ", p), err) }
  if err := oprot.WriteString(string(p.Message)); err != nil {
  return thrift.PrependError(fmt.Sprintf("%T.message (1) field write error: ", p), err) }
  if err := oprot.WriteFieldEnd(); err != nil {
    return thrift.PrependError(fmt.Sprintf("%T write field end error 1:message: ", p), err) }
  return err
}

func (p *ExceptionWithPrimitiveField) writeField2(oprot thrift.Protocol) (err error) {
  if err := oprot.WriteFieldBegin("error_code", thrift.I32, 2); err != nil {
    return thrift.PrependError(fmt.Sprintf("%T write field begin error 2:error_code: ", p), err) }
  if err := oprot.WriteI32(int32(p.ErrorCode)); err != nil {
  return thrift.PrependError(fmt.Sprintf("%T.error_code (2) field write error: ", p), err) }
  if err := oprot.WriteFieldEnd(); err != nil {
    return thrift.PrependError(fmt.Sprintf("%T write field end error 2:error_code: ", p), err) }
  return err
}

func (p *ExceptionWithPrimitiveField) String() string {
  if p == nil {
    return "<nil>"
  }

  messageVal := fmt.Sprintf("%v", p.Message)
  errorCodeVal := fmt.Sprintf("%v", p.ErrorCode)
  return fmt.Sprintf("ExceptionWithPrimitiveField({Message:%s ErrorCode:%s})", messageVal, errorCodeVal)
}

func (p *ExceptionWithPrimitiveField) Error() string {
  return p.String()
}

type Banal struct {
}

func NewBanal() *Banal {
  return &Banal{}
}

type BanalBuilder struct {
  obj *Banal
}

func NewBanalBuilder() *BanalBuilder{
  return &BanalBuilder{
    obj: NewBanal(),
  }
}

func (p BanalBuilder) Emit() *Banal{
  return &Banal{
  }
}

func (p *Banal) Read(iprot thrift.Protocol) error {
  if _, err := iprot.ReadStructBegin(); err != nil {
    return thrift.PrependError(fmt.Sprintf("%T read error: ", p), err)
  }


  for {
    _, fieldTypeId, fieldId, err := iprot.ReadFieldBegin()
    if err != nil {
      return thrift.PrependError(fmt.Sprintf("%T field %d read error: ", p, fieldId), err)
    }
    if fieldTypeId == thrift.STOP { break; }
    if err := iprot.Skip(fieldTypeId); err != nil {
      return err
    }
    if err := iprot.ReadFieldEnd(); err != nil {
      return err
    }
  }
  if err := iprot.ReadStructEnd(); err != nil {
    return thrift.PrependError(fmt.Sprintf("%T read struct end error: ", p), err)
  }
  return nil
}

func (p *Banal) Write(oprot thrift.Protocol) error {
  if err := oprot.WriteStructBegin("Banal"); err != nil {
    return thrift.PrependError(fmt.Sprintf("%T write struct begin error: ", p), err) }
  if err := oprot.WriteFieldStop(); err != nil {
    return thrift.PrependError("write field stop error: ", err) }
  if err := oprot.WriteStructEnd(); err != nil {
    return thrift.PrependError("write struct stop error: ", err) }
  return nil
}

func (p *Banal) String() string {
  if p == nil {
    return "<nil>"
  }

  return fmt.Sprintf("Banal({})")
}

func (p *Banal) Error() string {
  return p.String()
}

