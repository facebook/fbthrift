// @generated by Thrift for [[[ program path ]]]
// This file is probably not the place you want to edit!

package module // [[[ program thrift source path ]]]


import (
    "context"
    "fmt"
    "strings"

    thrift "github.com/facebook/fbthrift/thrift/lib/go/thrift"
    metadata "github.com/facebook/fbthrift/thrift/lib/thrift/metadata"
)

// (needed to ensure safety because of naive import list construction)
var _ = context.Background
var _ = fmt.Printf
var _ = strings.Split
var _ = thrift.ZERO
var _ = metadata.GoUnusedProtection__



type MyRoot interface {
    DoRoot(ctx context.Context) (error)
}

type MyRootChannelClientInterface interface {
    thrift.ClientInterface
    MyRoot
}

type MyRootClientInterface interface {
    thrift.ClientInterface
    DoRoot() (error)
}

type MyRootContextClientInterface interface {
    MyRootClientInterface
    DoRootContext(ctx context.Context) (error)
}

type MyRootChannelClient struct {
    ch thrift.RequestChannel
}
// Compile time interface enforcer
var _ MyRootChannelClientInterface = (*MyRootChannelClient)(nil)

func NewMyRootChannelClient(channel thrift.RequestChannel) *MyRootChannelClient {
    return &MyRootChannelClient{
        ch: channel,
    }
}

func (c *MyRootChannelClient) Close() error {
    return c.ch.Close()
}

type MyRootClient struct {
    chClient *MyRootChannelClient
}
// Compile time interface enforcer
var _ MyRootClientInterface = (*MyRootClient)(nil)
var _ MyRootContextClientInterface = (*MyRootClient)(nil)

func NewMyRootClient(prot thrift.Protocol) *MyRootClient {
    return &MyRootClient{
        chClient: NewMyRootChannelClient(
            thrift.NewSerialChannel(prot),
        ),
    }
}

func (c *MyRootClient) Close() error {
    return c.chClient.Close()
}

func (c *MyRootChannelClient) DoRoot(ctx context.Context) (error) {
    in := &reqMyRootDoRoot{
    }
    out := newRespMyRootDoRoot()
    err := c.ch.Call(ctx, "do_root", in, out)
    if err != nil {
        return err
    }
    return nil
}

func (c *MyRootClient) DoRoot() (error) {
    return c.chClient.DoRoot(context.Background())
}

func (c *MyRootClient) DoRootContext(ctx context.Context) (error) {
    return c.chClient.DoRoot(ctx)
}

type reqMyRootDoRoot struct {
}
// Compile time interface enforcer
var _ thrift.Struct = (*reqMyRootDoRoot)(nil)

// Deprecated: MyRootDoRootArgsDeprecated is deprecated, since it is supposed to be internal.
type MyRootDoRootArgsDeprecated = reqMyRootDoRoot

func newReqMyRootDoRoot() *reqMyRootDoRoot {
    return (&reqMyRootDoRoot{})
}



func (x *reqMyRootDoRoot) Write(p thrift.Format) error {
    if err := p.WriteStructBegin("reqMyRootDoRoot"); err != nil {
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

func (x *reqMyRootDoRoot) Read(p thrift.Format) error {
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

func (x *reqMyRootDoRoot) String() string {
    if x == nil {
        return "<nil>"
    }

    var sb strings.Builder

    sb.WriteString("reqMyRootDoRoot({")
    sb.WriteString("})")

    return sb.String()
}
type respMyRootDoRoot struct {
}
// Compile time interface enforcer
var _ thrift.Struct = (*respMyRootDoRoot)(nil)
var _ thrift.WritableResult = (*respMyRootDoRoot)(nil)

// Deprecated: MyRootDoRootResultDeprecated is deprecated, since it is supposed to be internal.
type MyRootDoRootResultDeprecated = respMyRootDoRoot

func newRespMyRootDoRoot() *respMyRootDoRoot {
    return (&respMyRootDoRoot{})
}



func (x *respMyRootDoRoot) Exception() thrift.WritableException {
    return nil
}

func (x *respMyRootDoRoot) Write(p thrift.Format) error {
    if err := p.WriteStructBegin("respMyRootDoRoot"); err != nil {
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

func (x *respMyRootDoRoot) Read(p thrift.Format) error {
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

func (x *respMyRootDoRoot) String() string {
    if x == nil {
        return "<nil>"
    }

    var sb strings.Builder

    sb.WriteString("respMyRootDoRoot({")
    sb.WriteString("})")

    return sb.String()
}


type MyRootProcessor struct {
    processorMap       map[string]thrift.ProcessorFunctionContext
    functionServiceMap map[string]string
    handler            MyRoot
}
// Compile time interface enforcer
var _ thrift.ProcessorContext = (*MyRootProcessor)(nil)

func NewMyRootProcessor(handler MyRoot) *MyRootProcessor {
    p := &MyRootProcessor{
        handler:            handler,
        processorMap:       make(map[string]thrift.ProcessorFunctionContext),
        functionServiceMap: make(map[string]string),
    }
    p.AddToProcessorMap("do_root", &procFuncMyRootDoRoot{handler: handler})
    p.AddToFunctionServiceMap("do_root", "MyRoot")

    return p
}

func (p *MyRootProcessor) AddToProcessorMap(key string, processor thrift.ProcessorFunctionContext) {
    p.processorMap[key] = processor
}

func (p *MyRootProcessor) AddToFunctionServiceMap(key, service string) {
    p.functionServiceMap[key] = service
}

func (p *MyRootProcessor) GetProcessorFunctionContext(key string) (processor thrift.ProcessorFunctionContext, err error) {
    if processor, ok := p.processorMap[key]; ok {
        return processor, nil
    }
    return nil, nil
}

func (p *MyRootProcessor) ProcessorMap() map[string]thrift.ProcessorFunctionContext {
    return p.processorMap
}

func (p *MyRootProcessor) FunctionServiceMap() map[string]string {
    return p.functionServiceMap
}

func (p *MyRootProcessor) GetThriftMetadata() *metadata.ThriftMetadata {
    return GetThriftMetadataForService("module.MyRoot")
}


type procFuncMyRootDoRoot struct {
    handler MyRoot
}
// Compile time interface enforcer
var _ thrift.ProcessorFunctionContext = (*procFuncMyRootDoRoot)(nil)

func (p *procFuncMyRootDoRoot) Read(iprot thrift.Format) (thrift.Struct, thrift.Exception) {
    args := newReqMyRootDoRoot()
    if err := args.Read(iprot); err != nil {
        return nil, err
    }
    iprot.ReadMessageEnd()
    return args, nil
}

func (p *procFuncMyRootDoRoot) Write(seqId int32, result thrift.WritableStruct, oprot thrift.Format) (err thrift.Exception) {
    var err2 error
    messageType := thrift.REPLY
    switch result.(type) {
    case thrift.ApplicationException:
        messageType = thrift.EXCEPTION
    }

    if err2 = oprot.WriteMessageBegin("do_root", messageType, seqId); err2 != nil {
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

func (p *procFuncMyRootDoRoot) RunContext(ctx context.Context, reqStruct thrift.Struct) (thrift.WritableStruct, thrift.ApplicationException) {
    result := newRespMyRootDoRoot()
    err := p.handler.DoRoot(ctx)
    if err != nil {
        x := thrift.NewApplicationExceptionCause(thrift.INTERNAL_ERROR, "Internal error processing DoRoot: " + err.Error(), err)
        return x, x
    }

    return result, nil
}




type MyNode interface {
    // Inherited/extended service
    MyRoot

    DoMid(ctx context.Context) (error)
}

type MyNodeChannelClientInterface interface {
    thrift.ClientInterface
    MyNode
}

type MyNodeClientInterface interface {
    thrift.ClientInterface
    DoMid() (error)
}

type MyNodeContextClientInterface interface {
    MyNodeClientInterface
    // Inherited/extended service
    MyRootContextClientInterface

    DoMidContext(ctx context.Context) (error)
}

type MyNodeChannelClient struct {
    // Inherited/extended service
    *MyRootChannelClient
    ch thrift.RequestChannel
}
// Compile time interface enforcer
var _ MyNodeChannelClientInterface = (*MyNodeChannelClient)(nil)

func NewMyNodeChannelClient(channel thrift.RequestChannel) *MyNodeChannelClient {
    return &MyNodeChannelClient{
        MyRootChannelClient: NewMyRootChannelClient(channel),
        ch: channel,
    }
}

func (c *MyNodeChannelClient) Close() error {
    return c.ch.Close()
}

type MyNodeClient struct {
    // Inherited/extended service
    *MyRootClient
    chClient *MyNodeChannelClient
}
// Compile time interface enforcer
var _ MyNodeClientInterface = (*MyNodeClient)(nil)
var _ MyNodeContextClientInterface = (*MyNodeClient)(nil)

func NewMyNodeClient(prot thrift.Protocol) *MyNodeClient {
    return &MyNodeClient{
        MyRootClient: NewMyRootClient(prot),
        chClient: NewMyNodeChannelClient(
            thrift.NewSerialChannel(prot),
        ),
    }
}

func (c *MyNodeClient) Close() error {
    return c.chClient.Close()
}

func (c *MyNodeChannelClient) DoMid(ctx context.Context) (error) {
    in := &reqMyNodeDoMid{
    }
    out := newRespMyNodeDoMid()
    err := c.ch.Call(ctx, "do_mid", in, out)
    if err != nil {
        return err
    }
    return nil
}

func (c *MyNodeClient) DoMid() (error) {
    return c.chClient.DoMid(context.Background())
}

func (c *MyNodeClient) DoMidContext(ctx context.Context) (error) {
    return c.chClient.DoMid(ctx)
}

type reqMyNodeDoMid struct {
}
// Compile time interface enforcer
var _ thrift.Struct = (*reqMyNodeDoMid)(nil)

// Deprecated: MyNodeDoMidArgsDeprecated is deprecated, since it is supposed to be internal.
type MyNodeDoMidArgsDeprecated = reqMyNodeDoMid

func newReqMyNodeDoMid() *reqMyNodeDoMid {
    return (&reqMyNodeDoMid{})
}



func (x *reqMyNodeDoMid) Write(p thrift.Format) error {
    if err := p.WriteStructBegin("reqMyNodeDoMid"); err != nil {
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

func (x *reqMyNodeDoMid) Read(p thrift.Format) error {
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

func (x *reqMyNodeDoMid) String() string {
    if x == nil {
        return "<nil>"
    }

    var sb strings.Builder

    sb.WriteString("reqMyNodeDoMid({")
    sb.WriteString("})")

    return sb.String()
}
type respMyNodeDoMid struct {
}
// Compile time interface enforcer
var _ thrift.Struct = (*respMyNodeDoMid)(nil)
var _ thrift.WritableResult = (*respMyNodeDoMid)(nil)

// Deprecated: MyNodeDoMidResultDeprecated is deprecated, since it is supposed to be internal.
type MyNodeDoMidResultDeprecated = respMyNodeDoMid

func newRespMyNodeDoMid() *respMyNodeDoMid {
    return (&respMyNodeDoMid{})
}



func (x *respMyNodeDoMid) Exception() thrift.WritableException {
    return nil
}

func (x *respMyNodeDoMid) Write(p thrift.Format) error {
    if err := p.WriteStructBegin("respMyNodeDoMid"); err != nil {
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

func (x *respMyNodeDoMid) Read(p thrift.Format) error {
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

func (x *respMyNodeDoMid) String() string {
    if x == nil {
        return "<nil>"
    }

    var sb strings.Builder

    sb.WriteString("respMyNodeDoMid({")
    sb.WriteString("})")

    return sb.String()
}


type MyNodeProcessor struct {
    // Inherited/extended processor
    *MyRootProcessor
}
// Compile time interface enforcer
var _ thrift.ProcessorContext = (*MyNodeProcessor)(nil)

func NewMyNodeProcessor(handler MyNode) *MyNodeProcessor {
    p := &MyNodeProcessor{
        NewMyRootProcessor(handler),
    }
    p.AddToProcessorMap("do_mid", &procFuncMyNodeDoMid{handler: handler})
    p.AddToFunctionServiceMap("do_mid", "MyNode")

    return p
}

func (p *MyNodeProcessor) GetThriftMetadata() *metadata.ThriftMetadata {
    return GetThriftMetadataForService("module.MyNode")
}


type procFuncMyNodeDoMid struct {
    handler MyNode
}
// Compile time interface enforcer
var _ thrift.ProcessorFunctionContext = (*procFuncMyNodeDoMid)(nil)

func (p *procFuncMyNodeDoMid) Read(iprot thrift.Format) (thrift.Struct, thrift.Exception) {
    args := newReqMyNodeDoMid()
    if err := args.Read(iprot); err != nil {
        return nil, err
    }
    iprot.ReadMessageEnd()
    return args, nil
}

func (p *procFuncMyNodeDoMid) Write(seqId int32, result thrift.WritableStruct, oprot thrift.Format) (err thrift.Exception) {
    var err2 error
    messageType := thrift.REPLY
    switch result.(type) {
    case thrift.ApplicationException:
        messageType = thrift.EXCEPTION
    }

    if err2 = oprot.WriteMessageBegin("do_mid", messageType, seqId); err2 != nil {
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

func (p *procFuncMyNodeDoMid) RunContext(ctx context.Context, reqStruct thrift.Struct) (thrift.WritableStruct, thrift.ApplicationException) {
    result := newRespMyNodeDoMid()
    err := p.handler.DoMid(ctx)
    if err != nil {
        x := thrift.NewApplicationExceptionCause(thrift.INTERNAL_ERROR, "Internal error processing DoMid: " + err.Error(), err)
        return x, x
    }

    return result, nil
}




type MyLeaf interface {
    // Inherited/extended service
    MyNode

    DoLeaf(ctx context.Context) (error)
}

type MyLeafChannelClientInterface interface {
    thrift.ClientInterface
    MyLeaf
}

type MyLeafClientInterface interface {
    thrift.ClientInterface
    DoLeaf() (error)
}

type MyLeafContextClientInterface interface {
    MyLeafClientInterface
    // Inherited/extended service
    MyNodeContextClientInterface

    DoLeafContext(ctx context.Context) (error)
}

type MyLeafChannelClient struct {
    // Inherited/extended service
    *MyNodeChannelClient
    ch thrift.RequestChannel
}
// Compile time interface enforcer
var _ MyLeafChannelClientInterface = (*MyLeafChannelClient)(nil)

func NewMyLeafChannelClient(channel thrift.RequestChannel) *MyLeafChannelClient {
    return &MyLeafChannelClient{
        MyNodeChannelClient: NewMyNodeChannelClient(channel),
        ch: channel,
    }
}

func (c *MyLeafChannelClient) Close() error {
    return c.ch.Close()
}

type MyLeafClient struct {
    // Inherited/extended service
    *MyNodeClient
    chClient *MyLeafChannelClient
}
// Compile time interface enforcer
var _ MyLeafClientInterface = (*MyLeafClient)(nil)
var _ MyLeafContextClientInterface = (*MyLeafClient)(nil)

func NewMyLeafClient(prot thrift.Protocol) *MyLeafClient {
    return &MyLeafClient{
        MyNodeClient: NewMyNodeClient(prot),
        chClient: NewMyLeafChannelClient(
            thrift.NewSerialChannel(prot),
        ),
    }
}

func (c *MyLeafClient) Close() error {
    return c.chClient.Close()
}

func (c *MyLeafChannelClient) DoLeaf(ctx context.Context) (error) {
    in := &reqMyLeafDoLeaf{
    }
    out := newRespMyLeafDoLeaf()
    err := c.ch.Call(ctx, "do_leaf", in, out)
    if err != nil {
        return err
    }
    return nil
}

func (c *MyLeafClient) DoLeaf() (error) {
    return c.chClient.DoLeaf(context.Background())
}

func (c *MyLeafClient) DoLeafContext(ctx context.Context) (error) {
    return c.chClient.DoLeaf(ctx)
}

type reqMyLeafDoLeaf struct {
}
// Compile time interface enforcer
var _ thrift.Struct = (*reqMyLeafDoLeaf)(nil)

// Deprecated: MyLeafDoLeafArgsDeprecated is deprecated, since it is supposed to be internal.
type MyLeafDoLeafArgsDeprecated = reqMyLeafDoLeaf

func newReqMyLeafDoLeaf() *reqMyLeafDoLeaf {
    return (&reqMyLeafDoLeaf{})
}



func (x *reqMyLeafDoLeaf) Write(p thrift.Format) error {
    if err := p.WriteStructBegin("reqMyLeafDoLeaf"); err != nil {
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

func (x *reqMyLeafDoLeaf) Read(p thrift.Format) error {
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

func (x *reqMyLeafDoLeaf) String() string {
    if x == nil {
        return "<nil>"
    }

    var sb strings.Builder

    sb.WriteString("reqMyLeafDoLeaf({")
    sb.WriteString("})")

    return sb.String()
}
type respMyLeafDoLeaf struct {
}
// Compile time interface enforcer
var _ thrift.Struct = (*respMyLeafDoLeaf)(nil)
var _ thrift.WritableResult = (*respMyLeafDoLeaf)(nil)

// Deprecated: MyLeafDoLeafResultDeprecated is deprecated, since it is supposed to be internal.
type MyLeafDoLeafResultDeprecated = respMyLeafDoLeaf

func newRespMyLeafDoLeaf() *respMyLeafDoLeaf {
    return (&respMyLeafDoLeaf{})
}



func (x *respMyLeafDoLeaf) Exception() thrift.WritableException {
    return nil
}

func (x *respMyLeafDoLeaf) Write(p thrift.Format) error {
    if err := p.WriteStructBegin("respMyLeafDoLeaf"); err != nil {
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

func (x *respMyLeafDoLeaf) Read(p thrift.Format) error {
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

func (x *respMyLeafDoLeaf) String() string {
    if x == nil {
        return "<nil>"
    }

    var sb strings.Builder

    sb.WriteString("respMyLeafDoLeaf({")
    sb.WriteString("})")

    return sb.String()
}


type MyLeafProcessor struct {
    // Inherited/extended processor
    *MyNodeProcessor
}
// Compile time interface enforcer
var _ thrift.ProcessorContext = (*MyLeafProcessor)(nil)

func NewMyLeafProcessor(handler MyLeaf) *MyLeafProcessor {
    p := &MyLeafProcessor{
        NewMyNodeProcessor(handler),
    }
    p.AddToProcessorMap("do_leaf", &procFuncMyLeafDoLeaf{handler: handler})
    p.AddToFunctionServiceMap("do_leaf", "MyLeaf")

    return p
}

func (p *MyLeafProcessor) GetThriftMetadata() *metadata.ThriftMetadata {
    return GetThriftMetadataForService("module.MyLeaf")
}


type procFuncMyLeafDoLeaf struct {
    handler MyLeaf
}
// Compile time interface enforcer
var _ thrift.ProcessorFunctionContext = (*procFuncMyLeafDoLeaf)(nil)

func (p *procFuncMyLeafDoLeaf) Read(iprot thrift.Format) (thrift.Struct, thrift.Exception) {
    args := newReqMyLeafDoLeaf()
    if err := args.Read(iprot); err != nil {
        return nil, err
    }
    iprot.ReadMessageEnd()
    return args, nil
}

func (p *procFuncMyLeafDoLeaf) Write(seqId int32, result thrift.WritableStruct, oprot thrift.Format) (err thrift.Exception) {
    var err2 error
    messageType := thrift.REPLY
    switch result.(type) {
    case thrift.ApplicationException:
        messageType = thrift.EXCEPTION
    }

    if err2 = oprot.WriteMessageBegin("do_leaf", messageType, seqId); err2 != nil {
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

func (p *procFuncMyLeafDoLeaf) RunContext(ctx context.Context, reqStruct thrift.Struct) (thrift.WritableStruct, thrift.ApplicationException) {
    result := newRespMyLeafDoLeaf()
    err := p.handler.DoLeaf(ctx)
    if err != nil {
        x := thrift.NewApplicationExceptionCause(thrift.INTERNAL_ERROR, "Internal error processing DoLeaf: " + err.Error(), err)
        return x, x
    }

    return result, nil
}


