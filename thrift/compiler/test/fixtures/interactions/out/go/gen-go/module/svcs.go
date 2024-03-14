// @generated by Thrift for [[[ program path ]]]
// This file is probably not the place you want to edit!

package module // [[[ program thrift source path ]]]


import (
    "context"
    "fmt"
    "strings"
    "sync"

    thrift "github.com/facebook/fbthrift/thrift/lib/go/thrift"
    metadata "github.com/facebook/fbthrift/thrift/lib/thrift/metadata"
)

// (needed to ensure safety because of naive import list construction)
var _ = context.Background
var _ = fmt.Printf
var _ = strings.Split
var _ = sync.Mutex{}
var _ = thrift.ZERO
var _ = metadata.GoUnusedProtection__



type MyService interface {
    Foo(ctx context.Context) (error)
}

type MyServiceChannelClientInterface interface {
    thrift.ClientInterface
    MyService
}

type MyServiceClientInterface interface {
    thrift.ClientInterface
    Foo() (error)
}

type MyServiceContextClientInterface interface {
    MyServiceClientInterface
    FooContext(ctx context.Context) (error)
}

type MyServiceChannelClient struct {
    ch thrift.RequestChannel
}
// Compile time interface enforcer
var _ MyServiceChannelClientInterface = &MyServiceChannelClient{}

func NewMyServiceChannelClient(channel thrift.RequestChannel) *MyServiceChannelClient {
    return &MyServiceChannelClient{
        ch: channel,
    }
}

func (c *MyServiceChannelClient) Close() error {
    return c.ch.Close()
}

type MyServiceClient struct {
    chClient *MyServiceChannelClient
    Mu       sync.Mutex
}
// Compile time interface enforcer
var _ MyServiceClientInterface = &MyServiceClient{}
var _ MyServiceContextClientInterface = &MyServiceClient{}

func NewMyServiceClient(prot thrift.Protocol) *MyServiceClient {
    return &MyServiceClient{
        chClient: NewMyServiceChannelClient(
            thrift.NewSerialChannel(prot),
        ),
    }
}

// Deprecated: NewMyServiceClientFromProtocol is deprecated rather call equivalent, but shorter function NewMyServiceClient.
func NewMyServiceClientFromProtocol(prot thrift.Protocol) *MyServiceClient {
    return &MyServiceClient{
        chClient: NewMyServiceChannelClient(
            thrift.NewSerialChannel(prot),
        ),
    }
}

func (c *MyServiceClient) Close() error {
    return c.chClient.Close()
}

func (c *MyServiceChannelClient) Foo(ctx context.Context) (error) {
    in := &reqMyServiceFoo{
    }
    out := newRespMyServiceFoo()
    err := c.ch.Call(ctx, "foo", in, out)
    if err != nil {
        return err
    }
    return nil
}

func (c *MyServiceClient) Foo() (error) {
    return c.chClient.Foo(context.Background())
}

func (c *MyServiceClient) FooContext(ctx context.Context) (error) {
    return c.chClient.Foo(ctx)
}

type reqMyServiceFoo struct {
}
// Compile time interface enforcer
var _ thrift.Struct = &reqMyServiceFoo{}

// Deprecated: MyServiceFooArgsDeprecated is deprecated, since it is supposed to be internal.
type MyServiceFooArgsDeprecated = reqMyServiceFoo

func newReqMyServiceFoo() *reqMyServiceFoo {
    return (&reqMyServiceFoo{})
}


// Deprecated: Use "New" constructor and setters to build your structs.
// e.g newReqMyServiceFoo().Set<FieldNameFoo>().Set<FieldNameBar>()
type reqMyServiceFooBuilder struct {
    obj *reqMyServiceFoo
}

// Deprecated: Use "New" constructor and setters to build your structs.
// e.g newReqMyServiceFoo().Set<FieldNameFoo>().Set<FieldNameBar>()
func newReqMyServiceFooBuilder() *reqMyServiceFooBuilder {
    return &reqMyServiceFooBuilder{
        obj: newReqMyServiceFoo(),
    }
}

// Deprecated: Use "New" constructor and setters to build your structs.
// e.g newReqMyServiceFoo().Set<FieldNameFoo>().Set<FieldNameBar>()
func (x *reqMyServiceFooBuilder) Emit() *reqMyServiceFoo {
    var objCopy reqMyServiceFoo = *x.obj
    return &objCopy
}

func (x *reqMyServiceFoo) Write(p thrift.Format) error {
    if err := p.WriteStructBegin("reqMyServiceFoo"); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write struct begin error: ", x), err)
    }

    if err := p.WriteFieldStop(); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write field stop error: ", x), err)
    }

    if err := p.WriteStructEnd(); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write struct end error: ", x), err)
    }
    return nil
}

func (x *reqMyServiceFoo) Read(p thrift.Format) error {
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

func (x *reqMyServiceFoo) String() string {
    if x == nil {
        return "<nil>"
    }

    var sb strings.Builder

    sb.WriteString("reqMyServiceFoo({")
    sb.WriteString("})")

    return sb.String()
}
type respMyServiceFoo struct {
}
// Compile time interface enforcer
var _ thrift.Struct = &respMyServiceFoo{}
var _ thrift.WritableResult = &respMyServiceFoo{}

// Deprecated: MyServiceFooResultDeprecated is deprecated, since it is supposed to be internal.
type MyServiceFooResultDeprecated = respMyServiceFoo

func newRespMyServiceFoo() *respMyServiceFoo {
    return (&respMyServiceFoo{})
}


// Deprecated: Use "New" constructor and setters to build your structs.
// e.g newRespMyServiceFoo().Set<FieldNameFoo>().Set<FieldNameBar>()
type respMyServiceFooBuilder struct {
    obj *respMyServiceFoo
}

// Deprecated: Use "New" constructor and setters to build your structs.
// e.g newRespMyServiceFoo().Set<FieldNameFoo>().Set<FieldNameBar>()
func newRespMyServiceFooBuilder() *respMyServiceFooBuilder {
    return &respMyServiceFooBuilder{
        obj: newRespMyServiceFoo(),
    }
}

// Deprecated: Use "New" constructor and setters to build your structs.
// e.g newRespMyServiceFoo().Set<FieldNameFoo>().Set<FieldNameBar>()
func (x *respMyServiceFooBuilder) Emit() *respMyServiceFoo {
    var objCopy respMyServiceFoo = *x.obj
    return &objCopy
}

func (x *respMyServiceFoo) Exception() thrift.WritableException {
    return nil
}

func (x *respMyServiceFoo) Write(p thrift.Format) error {
    if err := p.WriteStructBegin("respMyServiceFoo"); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write struct begin error: ", x), err)
    }

    if err := p.WriteFieldStop(); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write field stop error: ", x), err)
    }

    if err := p.WriteStructEnd(); err != nil {
        return thrift.PrependError(fmt.Sprintf("%T write struct end error: ", x), err)
    }
    return nil
}

func (x *respMyServiceFoo) Read(p thrift.Format) error {
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

func (x *respMyServiceFoo) String() string {
    if x == nil {
        return "<nil>"
    }

    var sb strings.Builder

    sb.WriteString("respMyServiceFoo({")
    sb.WriteString("})")

    return sb.String()
}


type MyServiceProcessor struct {
    processorMap       map[string]thrift.ProcessorFunctionContext
    functionServiceMap map[string]string
    handler            MyService
}
// Compile time interface enforcer
var _ thrift.ProcessorContext = &MyServiceProcessor{}

func NewMyServiceProcessor(handler MyService) *MyServiceProcessor {
    p := &MyServiceProcessor{
        handler:            handler,
        processorMap:       make(map[string]thrift.ProcessorFunctionContext),
        functionServiceMap: make(map[string]string),
    }
    p.AddToProcessorMap("foo", &procFuncMyServiceFoo{handler: handler})
    p.AddToFunctionServiceMap("foo", "MyService")

    return p
}

func (p *MyServiceProcessor) AddToProcessorMap(key string, processor thrift.ProcessorFunctionContext) {
    p.processorMap[key] = processor
}

func (p *MyServiceProcessor) AddToFunctionServiceMap(key, service string) {
    p.functionServiceMap[key] = service
}

func (p *MyServiceProcessor) GetProcessorFunctionContext(key string) (processor thrift.ProcessorFunctionContext, err error) {
    if processor, ok := p.processorMap[key]; ok {
        return processor, nil
    }
    return nil, nil
}

func (p *MyServiceProcessor) ProcessorMap() map[string]thrift.ProcessorFunctionContext {
    return p.processorMap
}

func (p *MyServiceProcessor) FunctionServiceMap() map[string]string {
    return p.functionServiceMap
}

func (p *MyServiceProcessor) GetThriftMetadata() *metadata.ThriftMetadata {
    return GetThriftMetadataForService("module.MyService")
}


type procFuncMyServiceFoo struct {
    handler MyService
}
// Compile time interface enforcer
var _ thrift.ProcessorFunctionContext = &procFuncMyServiceFoo{}

func (p *procFuncMyServiceFoo) Read(iprot thrift.Format) (thrift.Struct, thrift.Exception) {
    args := newReqMyServiceFoo()
    if err := args.Read(iprot); err != nil {
        return nil, err
    }
    iprot.ReadMessageEnd()
    return args, nil
}

func (p *procFuncMyServiceFoo) Write(seqId int32, result thrift.WritableStruct, oprot thrift.Format) (err thrift.Exception) {
    var err2 error
    messageType := thrift.REPLY
    switch result.(type) {
    case thrift.ApplicationException:
        messageType = thrift.EXCEPTION
    }

    if err2 = oprot.WriteMessageBegin("foo", messageType, seqId); err2 != nil {
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

func (p *procFuncMyServiceFoo) RunContext(ctx context.Context, reqStruct thrift.Struct) (thrift.WritableStruct, thrift.ApplicationException) {
    result := newRespMyServiceFoo()
    err := p.handler.Foo(ctx)
    if err != nil {
        x := thrift.NewApplicationExceptionCause(thrift.INTERNAL_ERROR, "Internal error processing Foo: " + err.Error(), err)
        return x, x
    }

    return result, nil
}

