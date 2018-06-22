package main

import (
	"fmt"
	"time"

	"apache/thrift/test/load"
	"github.com/golang/glog"
	"github.com/pkg/errors"
	"thrift/lib/go/thrift"
)

// Handler encapsulates parameters for your service processor
type Handler struct{}

// Serve initializes your thrift server
func Serve(addr string) error {
	handler := &Handler{}
	proc := load.NewLoadTestProcessor(handler)
	srv, err := newServer(proc, addr)

	if err != nil {
		return errors.Wrap(err, "failed to create thrift server")
	}

	glog.Infof("starting thrift server on '%s'", addr)
	return srv.Serve()
}

func newServer(processor thrift.Processor, addr string) (thrift.Server, error) {
	socket, err := thrift.NewServerSocket(addr)
	if err != nil {
		return nil, err
	}
	if err = socket.Listen(); err != nil {
		return nil, errors.Wrap(err, fmt.Sprintf("failed listen on %s", addr))
	}
	tFactory := thrift.NewHeaderTransportFactory(thrift.NewTransportFactory())
	pFactory := thrift.NewHeaderProtocolFactory()

	return thrift.NewSimpleServer(processor, socket, thrift.TransportFactories(tFactory), thrift.ProtocolFactories(pFactory)), nil
}

// Echo - does echo
func (h *Handler) Echo(in []byte) ([]byte, error) {
	return in, nil
}

// Add - adds
func (h *Handler) Add(a int64, b int64) (r int64, err error) {
	return a + b, nil
}

// Noop - a noop
func (h *Handler) Noop() (err error) {
	return nil
}

// OnewayNoop - a one way noop
func (h *Handler) OnewayNoop() (err error) {
	return nil
}

// AsyncNoop - a async noop
func (h *Handler) AsyncNoop() (err error) {
	return nil
}

// Sleep - a sleep
func (h *Handler) Sleep(microseconds int64) (err error) {
	time.Sleep(time.Duration(microseconds) * time.Microsecond)
	return nil
}

// OnewaySleep - a oneway sleep
func (h *Handler) OnewaySleep(microseconds int64) (err error) {
	time.Sleep(time.Duration(microseconds) * time.Microsecond)
	return nil
}

// Burn - a time burn
func (h *Handler) Burn(microseconds int64) (err error) {
	burnImpl(microseconds)
	return nil
}

// OnewayBurn - a one way time burn
func (h *Handler) OnewayBurn(microseconds int64) (err error) {
	burnImpl(microseconds)
	return nil
}

// BadSleep - a burn instead of a sleep
func (h *Handler) BadSleep(microseconds int64) (err error) {
	burnImpl(microseconds)
	return nil
}

// BadBurn - a time burn
func (h *Handler) BadBurn(microseconds int64) (err error) {
	burnImpl(microseconds)
	return nil
}

// ThrowError - throws a LoadError
func (h *Handler) ThrowError(code int32) (err error) {
	return load.NewLoadError()
}

// ThrowUnexpected - throws a ApplicationException
func (h *Handler) ThrowUnexpected(code int32) (err error) {
	return thrift.NewApplicationException(thrift.UNKNOWN_APPLICATION_EXCEPTION, "Unknown Exception")
}

// OnewayThrow - throws a ApplicationException
func (h *Handler) OnewayThrow(code int32) (err error) {
	return thrift.NewApplicationException(thrift.UNKNOWN_APPLICATION_EXCEPTION, "Unknown Exception")
}

// Send - does nothing
func (h *Handler) Send(data []byte) (err error) {
	return nil
}

// OnewaySend - does nothing
func (h *Handler) OnewaySend(data []byte) (err error) {
	return nil
}

// Recv - returns an empty byte array initialized to the size of the array
func (h *Handler) Recv(bytes int64) (r []byte, err error) {
	res := make([]byte, bytes)
	for n := range res {
		res[n] = byte('a')
	}
	return res, nil
}

// Sendrecv - returns a Recv invocation
func (h *Handler) Sendrecv(data []byte, recvBytes int64) (r []byte, err error) {
	if r, err = h.Recv(recvBytes); err != nil {
		return nil, err
	}
	return r, nil
}

// LargeContainer - does nothing
func (h *Handler) LargeContainer(items []*load.BigStruct) (err error) {
	return nil
}

// IterAllFields - iterates over all items and their fields
func (h *Handler) IterAllFields(items []*load.BigStruct) (r []*load.BigStruct, err error) {
	for _, item := range items {
		_ = item.GetStringField()
		for range item.GetStringList() {
		}
	}

	return items, nil
}

func burnImpl(microseconds int64) {
	end := time.Now().UnixNano() + microseconds*1000
	for time.Now().UnixNano() < end {
	}
}
