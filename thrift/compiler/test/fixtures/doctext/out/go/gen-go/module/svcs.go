// Autogenerated by Thrift for thrift/compiler/test/fixtures/doctext/src/module.thrift
//
// DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
//  @generated

package module


import (
    "context"
    "fmt"
    "io"
    "reflect"

    thrift "github.com/facebook/fbthrift/thrift/lib/go/thrift/types"
    metadata "github.com/facebook/fbthrift/thrift/lib/thrift/metadata"
)

// (needed to ensure safety because of naive import list construction)
var _ = context.Background
var _ = fmt.Printf
var _ = io.EOF
var _ = reflect.Ptr
var _ = thrift.VOID
var _ = metadata.GoUnusedProtection__

type C interface {
    F(ctx context.Context) (error)
    Thing(ctx context.Context, a int32, b string, c []int32) (string, error)
}

type CClientInterface interface {
    io.Closer
    C
}

type CClient struct {
    ch thrift.RequestChannel
}
// Compile time interface enforcer
var _ CClientInterface = (*CClient)(nil)

func NewCChannelClient(channel thrift.RequestChannel) *CClient {
    return &CClient{
        ch: channel,
    }
}

func NewCClient(prot thrift.DO_NOT_USE_ChannelWrapper) *CClient {
    var channel thrift.RequestChannel
    if prot != nil {
        channel = prot.DO_NOT_USE_WrapChannel()
    }
    return NewCChannelClient(channel)
}

func (c *CClient) Close() error {
    return c.ch.Close()
}

func (c *CClient) F(ctx context.Context) (error) {
    in := &reqCF{
    }
    out := newRespCF()
    err := c.ch.Call(ctx, "f", in, out)
    if err != nil {
        return err
    }
    return nil
}

func (c *CClient) Thing(ctx context.Context, a int32, b string, c []int32) (string, error) {
    in := &reqCThing{
        A: a,
        B: b,
        C: c,
    }
    out := newRespCThing()
    err := c.ch.Call(ctx, "thing", in, out)
    if err != nil {
        return "", err
    } else if out.Bang != nil {
        return "", out.Bang
    }
    return out.GetSuccess(), nil
}


type CProcessor struct {
    processorFunctionMap map[string]thrift.ProcessorFunction
    functionServiceMap   map[string]string
    handler            C
}

func NewCProcessor(handler C) *CProcessor {
    p := &CProcessor{
        handler:              handler,
        processorFunctionMap: make(map[string]thrift.ProcessorFunction),
        functionServiceMap:   make(map[string]string),
    }
    p.AddToProcessorFunctionMap("f", &procFuncCF{handler: handler})
    p.AddToProcessorFunctionMap("thing", &procFuncCThing{handler: handler})
    p.AddToFunctionServiceMap("f", "C")
    p.AddToFunctionServiceMap("thing", "C")

    return p
}

func (p *CProcessor) AddToProcessorFunctionMap(key string, processorFunction thrift.ProcessorFunction) {
    p.processorFunctionMap[key] = processorFunction
}

func (p *CProcessor) AddToFunctionServiceMap(key, service string) {
    p.functionServiceMap[key] = service
}

func (p *CProcessor) GetProcessorFunction(key string) (processor thrift.ProcessorFunction) {
    return p.processorFunctionMap[key]
}

func (p *CProcessor) ProcessorFunctionMap() map[string]thrift.ProcessorFunction {
    return p.processorFunctionMap
}

func (p *CProcessor) FunctionServiceMap() map[string]string {
    return p.functionServiceMap
}

func (p *CProcessor) PackageName() string {
    return "module"
}

func (p *CProcessor) GetThriftMetadata() *metadata.ThriftMetadata {
    return GetThriftMetadataForService("module.C")
}


type procFuncCF struct {
    handler C
}
// Compile time interface enforcer
var _ thrift.ProcessorFunction = (*procFuncCF)(nil)

func (p *procFuncCF) Read(iprot thrift.Decoder) (thrift.Struct, thrift.Exception) {
    args := newReqCF()
    if err := args.Read(iprot); err != nil {
        return nil, err
    }
    iprot.ReadMessageEnd()
    return args, nil
}

func (p *procFuncCF) Write(seqId int32, result thrift.WritableStruct, oprot thrift.Encoder) (err thrift.Exception) {
    var err2 error
    messageType := thrift.REPLY
    switch result.(type) {
    case thrift.ApplicationException:
        messageType = thrift.EXCEPTION
    }

    if err2 = oprot.WriteMessageBegin("f", messageType, seqId); err2 != nil {
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

func (p *procFuncCF) RunContext(ctx context.Context, reqStruct thrift.Struct) (thrift.WritableStruct, thrift.ApplicationException) {
    result := newRespCF()
    err := p.handler.F(ctx)
    if err != nil {
        x := thrift.NewApplicationExceptionCause(thrift.INTERNAL_ERROR, "Internal error processing F: " + err.Error(), err)
        return x, x
    }

    return result, nil
}


type procFuncCThing struct {
    handler C
}
// Compile time interface enforcer
var _ thrift.ProcessorFunction = (*procFuncCThing)(nil)

func (p *procFuncCThing) Read(iprot thrift.Decoder) (thrift.Struct, thrift.Exception) {
    args := newReqCThing()
    if err := args.Read(iprot); err != nil {
        return nil, err
    }
    iprot.ReadMessageEnd()
    return args, nil
}

func (p *procFuncCThing) Write(seqId int32, result thrift.WritableStruct, oprot thrift.Encoder) (err thrift.Exception) {
    var err2 error
    messageType := thrift.REPLY
    switch v := result.(type) {
    case *Bang:
        result = &respCThing{
            Bang: v,
        }
    case thrift.ApplicationException:
        messageType = thrift.EXCEPTION
    }

    if err2 = oprot.WriteMessageBegin("thing", messageType, seqId); err2 != nil {
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

func (p *procFuncCThing) RunContext(ctx context.Context, reqStruct thrift.Struct) (thrift.WritableStruct, thrift.ApplicationException) {
    args := reqStruct.(*reqCThing)
    result := newRespCThing()
    retval, err := p.handler.Thing(ctx, args.A, args.B, args.C)
    if err != nil {
        switch v := err.(type) {
        case *Bang:
            result.Bang = v
            return result, nil
        default:
            x := thrift.NewApplicationExceptionCause(thrift.INTERNAL_ERROR, "Internal error processing Thing: " + err.Error(), err)
            return x, x
        }
    }

    result.Success = &retval
    return result, nil
}


