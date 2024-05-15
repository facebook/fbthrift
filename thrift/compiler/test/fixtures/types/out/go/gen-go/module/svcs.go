// @generated by Thrift for [[[ program path ]]]
// This file is probably not the place you want to edit!

package module // [[[ program thrift source path ]]]


import (
    "context"
    "fmt"
    "strings"
    "sync"

    included "included"
    thrift "github.com/facebook/fbthrift/thrift/lib/go/thrift"
    metadata "github.com/facebook/fbthrift/thrift/lib/thrift/metadata"
)

var _ = included.GoUnusedProtection__
// (needed to ensure safety because of naive import list construction)
var _ = context.Background
var _ = fmt.Printf
var _ = strings.Split
var _ = sync.Mutex{}
var _ = thrift.ZERO
var _ = metadata.GoUnusedProtection__



type SomeService interface {
    BounceMap(ctx context.Context, m included.SomeMap) (included.SomeMap, error)
    BinaryKeyedMap(ctx context.Context, r []int64) (map[*TBinary]int64, error)
}

type SomeServiceChannelClientInterface interface {
    thrift.ClientInterface
    SomeService
}

type SomeServiceClientInterface interface {
    thrift.ClientInterface
    BounceMap(m included.SomeMap) (included.SomeMap, error)
    BinaryKeyedMap(r []int64) (map[*TBinary]int64, error)
}

type SomeServiceContextClientInterface interface {
    SomeServiceClientInterface
    BounceMapContext(ctx context.Context, m included.SomeMap) (included.SomeMap, error)
    BinaryKeyedMapContext(ctx context.Context, r []int64) (map[*TBinary]int64, error)
}

type SomeServiceChannelClient struct {
    ch thrift.RequestChannel
}
// Compile time interface enforcer
var _ SomeServiceChannelClientInterface = &SomeServiceChannelClient{}

func NewSomeServiceChannelClient(channel thrift.RequestChannel) *SomeServiceChannelClient {
    return &SomeServiceChannelClient{
        ch: channel,
    }
}

func (c *SomeServiceChannelClient) Close() error {
    return c.ch.Close()
}

type SomeServiceClient struct {
    chClient *SomeServiceChannelClient
    Mu       sync.Mutex
}
// Compile time interface enforcer
var _ SomeServiceClientInterface = &SomeServiceClient{}
var _ SomeServiceContextClientInterface = &SomeServiceClient{}

func NewSomeServiceClient(prot thrift.Protocol) *SomeServiceClient {
    return &SomeServiceClient{
        chClient: NewSomeServiceChannelClient(
            thrift.NewSerialChannel(prot),
        ),
    }
}

func (c *SomeServiceClient) Close() error {
    return c.chClient.Close()
}

func (c *SomeServiceChannelClient) BounceMap(ctx context.Context, m included.SomeMap) (included.SomeMap, error) {
    in := &reqSomeServiceBounceMap{
        M: m,
    }
    out := newRespSomeServiceBounceMap()
    err := c.ch.Call(ctx, "bounce_map", in, out)
    if err != nil {
        return nil, err
    }
    return out.GetSuccess(), nil
}

func (c *SomeServiceClient) BounceMap(m included.SomeMap) (included.SomeMap, error) {
    return c.chClient.BounceMap(context.Background(), m)
}

func (c *SomeServiceClient) BounceMapContext(ctx context.Context, m included.SomeMap) (included.SomeMap, error) {
    return c.chClient.BounceMap(ctx, m)
}

func (c *SomeServiceChannelClient) BinaryKeyedMap(ctx context.Context, r []int64) (map[*TBinary]int64, error) {
    in := &reqSomeServiceBinaryKeyedMap{
        R: r,
    }
    out := newRespSomeServiceBinaryKeyedMap()
    err := c.ch.Call(ctx, "binary_keyed_map", in, out)
    if err != nil {
        return nil, err
    }
    return out.GetSuccess(), nil
}

func (c *SomeServiceClient) BinaryKeyedMap(r []int64) (map[*TBinary]int64, error) {
    return c.chClient.BinaryKeyedMap(context.Background(), r)
}

func (c *SomeServiceClient) BinaryKeyedMapContext(ctx context.Context, r []int64) (map[*TBinary]int64, error) {
    return c.chClient.BinaryKeyedMap(ctx, r)
}

type reqSomeServiceBounceMap struct {
    M included.SomeMap `thrift:"m,1" json:"m" db:"m"`
}
// Compile time interface enforcer
var _ thrift.Struct = &reqSomeServiceBounceMap{}

// Deprecated: SomeServiceBounceMapArgsDeprecated is deprecated, since it is supposed to be internal.
type SomeServiceBounceMapArgsDeprecated = reqSomeServiceBounceMap

func newReqSomeServiceBounceMap() *reqSomeServiceBounceMap {
    return (&reqSomeServiceBounceMap{}).
        SetMNonCompat(included.NewSomeMap())
}

func (x *reqSomeServiceBounceMap) GetMNonCompat() included.SomeMap {
    return x.M
}

func (x *reqSomeServiceBounceMap) GetM() included.SomeMap {
    if !x.IsSetM() {
        return included.NewSomeMap()
    }

    return x.M
}

func (x *reqSomeServiceBounceMap) SetMNonCompat(value included.SomeMap) *reqSomeServiceBounceMap {
    x.M = value
    return x
}

func (x *reqSomeServiceBounceMap) SetM(value included.SomeMap) *reqSomeServiceBounceMap {
    x.M = value
    return x
}

func (x *reqSomeServiceBounceMap) IsSetM() bool {
    return x != nil && x.M != nil
}

func (x *reqSomeServiceBounceMap) writeField1(p thrift.Format) error {  // M
    if err := p.WriteFieldBegin("m", thrift.MAP, 1); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write field begin error: ", x), err)
    }

    item := x.M
    err := included.WriteSomeMap(item, p)
if err != nil {
    return err
}

    if err := p.WriteFieldEnd(); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write field end error: ", x), err)
    }
    return nil
}

func (x *reqSomeServiceBounceMap) readField1(p thrift.Format) error {  // M
    result, err := included.ReadSomeMap(p)
if err != nil {
    return err
}

    x.SetMNonCompat(result)
    return nil
}

func (x *reqSomeServiceBounceMap) toString1() string {  // M
    return fmt.Sprintf("%v", x.M)
}



func (x *reqSomeServiceBounceMap) Write(p thrift.Format) error {
    if err := p.WriteStructBegin("reqSomeServiceBounceMap"); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write struct begin error: ", x), err)
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

func (x *reqSomeServiceBounceMap) Read(p thrift.Format) error {
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
        case (id == 1 && wireType == thrift.Type(thrift.MAP)):  // m
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

func (x *reqSomeServiceBounceMap) String() string {
    if x == nil {
        return "<nil>"
    }

    var sb strings.Builder

    sb.WriteString("reqSomeServiceBounceMap({")
    sb.WriteString(fmt.Sprintf("M:%s", x.toString1()))
    sb.WriteString("})")

    return sb.String()
}
type respSomeServiceBounceMap struct {
    Success included.SomeMap `thrift:"success,0,optional" json:"success,omitempty" db:"success"`
}
// Compile time interface enforcer
var _ thrift.Struct = &respSomeServiceBounceMap{}
var _ thrift.WritableResult = &respSomeServiceBounceMap{}

// Deprecated: SomeServiceBounceMapResultDeprecated is deprecated, since it is supposed to be internal.
type SomeServiceBounceMapResultDeprecated = respSomeServiceBounceMap

func newRespSomeServiceBounceMap() *respSomeServiceBounceMap {
    return (&respSomeServiceBounceMap{})
}

func (x *respSomeServiceBounceMap) GetSuccessNonCompat() included.SomeMap {
    return x.Success
}

func (x *respSomeServiceBounceMap) GetSuccess() included.SomeMap {
    if !x.IsSetSuccess() {
        return included.NewSomeMap()
    }

    return x.Success
}

func (x *respSomeServiceBounceMap) SetSuccessNonCompat(value included.SomeMap) *respSomeServiceBounceMap {
    x.Success = value
    return x
}

func (x *respSomeServiceBounceMap) SetSuccess(value included.SomeMap) *respSomeServiceBounceMap {
    x.Success = value
    return x
}

func (x *respSomeServiceBounceMap) IsSetSuccess() bool {
    return x != nil && x.Success != nil
}

func (x *respSomeServiceBounceMap) writeField0(p thrift.Format) error {  // Success
    if !x.IsSetSuccess() {
        return nil
    }

    if err := p.WriteFieldBegin("success", thrift.MAP, 0); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write field begin error: ", x), err)
    }

    item := x.Success
    err := included.WriteSomeMap(item, p)
if err != nil {
    return err
}

    if err := p.WriteFieldEnd(); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write field end error: ", x), err)
    }
    return nil
}

func (x *respSomeServiceBounceMap) readField0(p thrift.Format) error {  // Success
    result, err := included.ReadSomeMap(p)
if err != nil {
    return err
}

    x.SetSuccessNonCompat(result)
    return nil
}

func (x *respSomeServiceBounceMap) toString0() string {  // Success
    return fmt.Sprintf("%v", x.Success)
}



func (x *respSomeServiceBounceMap) Exception() thrift.WritableException {
    return nil
}

func (x *respSomeServiceBounceMap) Write(p thrift.Format) error {
    if err := p.WriteStructBegin("respSomeServiceBounceMap"); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write struct begin error: ", x), err)
    }

    if err := x.writeField0(p); err != nil {
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

func (x *respSomeServiceBounceMap) Read(p thrift.Format) error {
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
        case (id == 0 && wireType == thrift.Type(thrift.MAP)):  // success
            if err := x.readField0(p); err != nil {
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

func (x *respSomeServiceBounceMap) String() string {
    if x == nil {
        return "<nil>"
    }

    var sb strings.Builder

    sb.WriteString("respSomeServiceBounceMap({")
    sb.WriteString(fmt.Sprintf("Success:%s", x.toString0()))
    sb.WriteString("})")

    return sb.String()
}
type reqSomeServiceBinaryKeyedMap struct {
    R []int64 `thrift:"r,1" json:"r" db:"r"`
}
// Compile time interface enforcer
var _ thrift.Struct = &reqSomeServiceBinaryKeyedMap{}

// Deprecated: SomeServiceBinaryKeyedMapArgsDeprecated is deprecated, since it is supposed to be internal.
type SomeServiceBinaryKeyedMapArgsDeprecated = reqSomeServiceBinaryKeyedMap

func newReqSomeServiceBinaryKeyedMap() *reqSomeServiceBinaryKeyedMap {
    return (&reqSomeServiceBinaryKeyedMap{}).
        SetRNonCompat(make([]int64, 0))
}

func (x *reqSomeServiceBinaryKeyedMap) GetRNonCompat() []int64 {
    return x.R
}

func (x *reqSomeServiceBinaryKeyedMap) GetR() []int64 {
    if !x.IsSetR() {
        return make([]int64, 0)
    }

    return x.R
}

func (x *reqSomeServiceBinaryKeyedMap) SetRNonCompat(value []int64) *reqSomeServiceBinaryKeyedMap {
    x.R = value
    return x
}

func (x *reqSomeServiceBinaryKeyedMap) SetR(value []int64) *reqSomeServiceBinaryKeyedMap {
    x.R = value
    return x
}

func (x *reqSomeServiceBinaryKeyedMap) IsSetR() bool {
    return x != nil && x.R != nil
}

func (x *reqSomeServiceBinaryKeyedMap) writeField1(p thrift.Format) error {  // R
    if err := p.WriteFieldBegin("r", thrift.LIST, 1); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write field begin error: ", x), err)
    }

    item := x.R
    if err := p.WriteListBegin(thrift.I64, len(item)); err != nil {
    return thrift.PrependError("error writing list begin: ", err)
}
for _, v := range item {
    {
        item := v
        if err := p.WriteI64(item); err != nil {
    return err
}
    }
}
if err := p.WriteListEnd(); err != nil {
    return thrift.PrependError("error writing list end: ", err)
}

    if err := p.WriteFieldEnd(); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write field end error: ", x), err)
    }
    return nil
}

func (x *reqSomeServiceBinaryKeyedMap) readField1(p thrift.Format) error {  // R
    _ /* elemType */, size, err := p.ReadListBegin()
if err != nil {
    return thrift.PrependError("error reading list begin: ", err)
}

listResult := make([]int64, 0, size)
for i := 0; i < size; i++ {
    var elem int64
    {
        result, err := p.ReadI64()
if err != nil {
    return err
}
        elem = result
    }
    listResult = append(listResult, elem)
}

if err := p.ReadListEnd(); err != nil {
    return thrift.PrependError("error reading list end: ", err)
}
result := listResult

    x.SetRNonCompat(result)
    return nil
}

func (x *reqSomeServiceBinaryKeyedMap) toString1() string {  // R
    return fmt.Sprintf("%v", x.R)
}



func (x *reqSomeServiceBinaryKeyedMap) Write(p thrift.Format) error {
    if err := p.WriteStructBegin("reqSomeServiceBinaryKeyedMap"); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write struct begin error: ", x), err)
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

func (x *reqSomeServiceBinaryKeyedMap) Read(p thrift.Format) error {
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
        case (id == 1 && wireType == thrift.Type(thrift.LIST)):  // r
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

func (x *reqSomeServiceBinaryKeyedMap) String() string {
    if x == nil {
        return "<nil>"
    }

    var sb strings.Builder

    sb.WriteString("reqSomeServiceBinaryKeyedMap({")
    sb.WriteString(fmt.Sprintf("R:%s", x.toString1()))
    sb.WriteString("})")

    return sb.String()
}
type respSomeServiceBinaryKeyedMap struct {
    Success map[*TBinary]int64 `thrift:"success,0,optional" json:"success,omitempty" db:"success"`
}
// Compile time interface enforcer
var _ thrift.Struct = &respSomeServiceBinaryKeyedMap{}
var _ thrift.WritableResult = &respSomeServiceBinaryKeyedMap{}

// Deprecated: SomeServiceBinaryKeyedMapResultDeprecated is deprecated, since it is supposed to be internal.
type SomeServiceBinaryKeyedMapResultDeprecated = respSomeServiceBinaryKeyedMap

func newRespSomeServiceBinaryKeyedMap() *respSomeServiceBinaryKeyedMap {
    return (&respSomeServiceBinaryKeyedMap{})
}

func (x *respSomeServiceBinaryKeyedMap) GetSuccessNonCompat() map[*TBinary]int64 {
    return x.Success
}

func (x *respSomeServiceBinaryKeyedMap) GetSuccess() map[*TBinary]int64 {
    if !x.IsSetSuccess() {
        return make(map[*TBinary]int64)
    }

    return x.Success
}

func (x *respSomeServiceBinaryKeyedMap) SetSuccessNonCompat(value map[*TBinary]int64) *respSomeServiceBinaryKeyedMap {
    x.Success = value
    return x
}

func (x *respSomeServiceBinaryKeyedMap) SetSuccess(value map[*TBinary]int64) *respSomeServiceBinaryKeyedMap {
    x.Success = value
    return x
}

func (x *respSomeServiceBinaryKeyedMap) IsSetSuccess() bool {
    return x != nil && x.Success != nil
}

func (x *respSomeServiceBinaryKeyedMap) writeField0(p thrift.Format) error {  // Success
    if !x.IsSetSuccess() {
        return nil
    }

    if err := p.WriteFieldBegin("success", thrift.MAP, 0); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write field begin error: ", x), err)
    }

    item := x.Success
    if err := p.WriteMapBegin(thrift.STRING, thrift.I64, len(item)); err != nil {
    return thrift.PrependError("error writing map begin: ", err)
}
for k, v := range item {
    if k == nil {
        return fmt.Errorf("%T write map: nil key is not allowed.", item)
    }

    {
        item := *k
        err := WriteTBinary(item, p)
if err != nil {
    return err
}
    }

    {
        item := v
        if err := p.WriteI64(item); err != nil {
    return err
}
    }
}
if err := p.WriteMapEnd(); err != nil {
    return thrift.PrependError("error writing map end: ", err)
}

    if err := p.WriteFieldEnd(); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write field end error: ", x), err)
    }
    return nil
}

func (x *respSomeServiceBinaryKeyedMap) readField0(p thrift.Format) error {  // Success
    _ /* keyType */, _ /* valueType */, size, err := p.ReadMapBegin()
if err != nil {
    return thrift.PrependError("error reading map begin: ", err)
}

mapResult := make(map[*TBinary]int64, size)
for i := 0; i < size; i++ {
    var key *TBinary
    {
        result, err := ReadTBinary(p)
if err != nil {
    return err
}
        key = &result
    }

    var value int64
    {
        result, err := p.ReadI64()
if err != nil {
    return err
}
        value = result
    }

    mapResult[key] = value
}

if err := p.ReadMapEnd(); err != nil {
    return thrift.PrependError("error reading map end: ", err)
}
result := mapResult

    x.SetSuccessNonCompat(result)
    return nil
}

func (x *respSomeServiceBinaryKeyedMap) toString0() string {  // Success
    return fmt.Sprintf("%v", x.Success)
}



func (x *respSomeServiceBinaryKeyedMap) Exception() thrift.WritableException {
    return nil
}

func (x *respSomeServiceBinaryKeyedMap) Write(p thrift.Format) error {
    if err := p.WriteStructBegin("respSomeServiceBinaryKeyedMap"); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write struct begin error: ", x), err)
    }

    if err := x.writeField0(p); err != nil {
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

func (x *respSomeServiceBinaryKeyedMap) Read(p thrift.Format) error {
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
        case (id == 0 && wireType == thrift.Type(thrift.MAP)):  // success
            if err := x.readField0(p); err != nil {
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

func (x *respSomeServiceBinaryKeyedMap) String() string {
    if x == nil {
        return "<nil>"
    }

    var sb strings.Builder

    sb.WriteString("respSomeServiceBinaryKeyedMap({")
    sb.WriteString(fmt.Sprintf("Success:%s", x.toString0()))
    sb.WriteString("})")

    return sb.String()
}


type SomeServiceProcessor struct {
    processorMap       map[string]thrift.ProcessorFunctionContext
    functionServiceMap map[string]string
    handler            SomeService
}
// Compile time interface enforcer
var _ thrift.ProcessorContext = &SomeServiceProcessor{}

func NewSomeServiceProcessor(handler SomeService) *SomeServiceProcessor {
    p := &SomeServiceProcessor{
        handler:            handler,
        processorMap:       make(map[string]thrift.ProcessorFunctionContext),
        functionServiceMap: make(map[string]string),
    }
    p.AddToProcessorMap("bounce_map", &procFuncSomeServiceBounceMap{handler: handler})
    p.AddToProcessorMap("binary_keyed_map", &procFuncSomeServiceBinaryKeyedMap{handler: handler})
    p.AddToFunctionServiceMap("bounce_map", "SomeService")
    p.AddToFunctionServiceMap("binary_keyed_map", "SomeService")

    return p
}

func (p *SomeServiceProcessor) AddToProcessorMap(key string, processor thrift.ProcessorFunctionContext) {
    p.processorMap[key] = processor
}

func (p *SomeServiceProcessor) AddToFunctionServiceMap(key, service string) {
    p.functionServiceMap[key] = service
}

func (p *SomeServiceProcessor) GetProcessorFunctionContext(key string) (processor thrift.ProcessorFunctionContext, err error) {
    if processor, ok := p.processorMap[key]; ok {
        return processor, nil
    }
    return nil, nil
}

func (p *SomeServiceProcessor) ProcessorMap() map[string]thrift.ProcessorFunctionContext {
    return p.processorMap
}

func (p *SomeServiceProcessor) FunctionServiceMap() map[string]string {
    return p.functionServiceMap
}

func (p *SomeServiceProcessor) GetThriftMetadata() *metadata.ThriftMetadata {
    return GetThriftMetadataForService("module.SomeService")
}


type procFuncSomeServiceBounceMap struct {
    handler SomeService
}
// Compile time interface enforcer
var _ thrift.ProcessorFunctionContext = &procFuncSomeServiceBounceMap{}

func (p *procFuncSomeServiceBounceMap) Read(iprot thrift.Format) (thrift.Struct, thrift.Exception) {
    args := newReqSomeServiceBounceMap()
    if err := args.Read(iprot); err != nil {
        return nil, err
    }
    iprot.ReadMessageEnd()
    return args, nil
}

func (p *procFuncSomeServiceBounceMap) Write(seqId int32, result thrift.WritableStruct, oprot thrift.Format) (err thrift.Exception) {
    var err2 error
    messageType := thrift.REPLY
    switch result.(type) {
    case thrift.ApplicationException:
        messageType = thrift.EXCEPTION
    }

    if err2 = oprot.WriteMessageBegin("bounce_map", messageType, seqId); err2 != nil {
        err = err2
    }
    if err2 = result.Write(oprot); err == nil && err2 != nil {
        err = err2
    }
    if err2 = oprot.WriteMessageEnd(); err == nil && err2 != nil {
        err = err2
    }
    if err2 = oprot.Flush(); err == nil && err2 != nil {
        err = err2
    }
    return err
}

func (p *procFuncSomeServiceBounceMap) RunContext(ctx context.Context, reqStruct thrift.Struct) (thrift.WritableStruct, thrift.ApplicationException) {
    args := reqStruct.(*reqSomeServiceBounceMap)
    result := newRespSomeServiceBounceMap()
    retval, err := p.handler.BounceMap(ctx, args.M)
    if err != nil {
        x := thrift.NewApplicationExceptionCause(thrift.INTERNAL_ERROR, "Internal error processing BounceMap: " + err.Error(), err)
        return x, x
    }

    result.Success = retval
    return result, nil
}


type procFuncSomeServiceBinaryKeyedMap struct {
    handler SomeService
}
// Compile time interface enforcer
var _ thrift.ProcessorFunctionContext = &procFuncSomeServiceBinaryKeyedMap{}

func (p *procFuncSomeServiceBinaryKeyedMap) Read(iprot thrift.Format) (thrift.Struct, thrift.Exception) {
    args := newReqSomeServiceBinaryKeyedMap()
    if err := args.Read(iprot); err != nil {
        return nil, err
    }
    iprot.ReadMessageEnd()
    return args, nil
}

func (p *procFuncSomeServiceBinaryKeyedMap) Write(seqId int32, result thrift.WritableStruct, oprot thrift.Format) (err thrift.Exception) {
    var err2 error
    messageType := thrift.REPLY
    switch result.(type) {
    case thrift.ApplicationException:
        messageType = thrift.EXCEPTION
    }

    if err2 = oprot.WriteMessageBegin("binary_keyed_map", messageType, seqId); err2 != nil {
        err = err2
    }
    if err2 = result.Write(oprot); err == nil && err2 != nil {
        err = err2
    }
    if err2 = oprot.WriteMessageEnd(); err == nil && err2 != nil {
        err = err2
    }
    if err2 = oprot.Flush(); err == nil && err2 != nil {
        err = err2
    }
    return err
}

func (p *procFuncSomeServiceBinaryKeyedMap) RunContext(ctx context.Context, reqStruct thrift.Struct) (thrift.WritableStruct, thrift.ApplicationException) {
    args := reqStruct.(*reqSomeServiceBinaryKeyedMap)
    result := newRespSomeServiceBinaryKeyedMap()
    retval, err := p.handler.BinaryKeyedMap(ctx, args.R)
    if err != nil {
        x := thrift.NewApplicationExceptionCause(thrift.INTERNAL_ERROR, "Internal error processing BinaryKeyedMap: " + err.Error(), err)
        return x, x
    }

    result.Success = retval
    return result, nil
}


