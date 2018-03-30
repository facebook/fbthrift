/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

package thrift

import (
	"context"
	"errors"
	"log"
	"runtime/debug"
)

// ErrServerClosed is returned by the Serve methods after a call to Stop
var ErrServerClosed = errors.New("thrift: Server closed")

// SimpleServer is a simple, non-concurrent server for testing.
type SimpleServer struct {
	quit chan struct{}
	log  *log.Logger

	processorFactory       ProcessorFactory
	serverTransport        ServerTransport
	inputTransportFactory  TransportFactory
	outputTransportFactory TransportFactory
	inputProtocolFactory   ProtocolFactory
	outputProtocolFactory  ProtocolFactory
}

// TransportFactories sets both input and output transport factories
func TransportFactories(factory TransportFactory) func(*SimpleServer) {
	return func(server *SimpleServer) {
		server.inputTransportFactory = factory
		server.outputTransportFactory = factory
	}
}

// InputTransportFactory sets the input transport factory
func InputTransportFactory(factory TransportFactory) func(*SimpleServer) {
	return func(server *SimpleServer) {
		server.inputTransportFactory = factory
	}
}

// OutputTransportFactory sets the output transport factory
func OutputTransportFactory(factory TransportFactory) func(*SimpleServer) {
	return func(server *SimpleServer) {
		server.outputTransportFactory = factory
	}
}

// ProtocolFactories sets both input and output protocol factories
func ProtocolFactories(factory ProtocolFactory) func(*SimpleServer) {
	return func(server *SimpleServer) {
		server.inputProtocolFactory = factory
		server.outputProtocolFactory = factory
	}
}

// InputProtocolFactory sets the input protocol factory
func InputProtocolFactory(factory ProtocolFactory) func(*SimpleServer) {
	return func(server *SimpleServer) {
		server.inputProtocolFactory = factory
	}
}

// OutputProtocolFactory sets the output protocol factory
func OutputProtocolFactory(factory ProtocolFactory) func(*SimpleServer) {
	return func(server *SimpleServer) {
		server.outputProtocolFactory = factory
	}
}

// Logger sets the logger used for the server
func Logger(log *log.Logger) func(*SimpleServer) {
	return func(server *SimpleServer) {
		server.log = log
	}
}

// NewSimpleServer create a new server
func NewSimpleServer(processor Processor, serverTransport ServerTransport, options ...func(*SimpleServer)) *SimpleServer {
	return NewSimpleServerFactory(NewProcessorFactory(processor), serverTransport, options...)
}

// NewSimpleServer2 is deprecated, used NewSimpleServer instead
func NewSimpleServer2(processor Processor, serverTransport ServerTransport) *SimpleServer {
	return NewSimpleServerFactory(NewProcessorFactory(processor), serverTransport)
}

// NewSimpleServer4 is deprecated, used NewSimpleServer instead
func NewSimpleServer4(processor Processor, serverTransport ServerTransport, transportFactory TransportFactory, protocolFactory ProtocolFactory) *SimpleServer {
	return NewSimpleServerFactory(
		NewProcessorFactory(processor),
		serverTransport,
		TransportFactories(transportFactory),
		ProtocolFactories(protocolFactory),
	)
}

// NewSimpleServer6 is deprecated, used NewSimpleServer instead
func NewSimpleServer6(processor Processor, serverTransport ServerTransport, inputTransportFactory TransportFactory, outputTransportFactory TransportFactory, inputProtocolFactory ProtocolFactory, outputProtocolFactory ProtocolFactory) *SimpleServer {
	return NewSimpleServerFactory(
		NewProcessorFactory(processor),
		serverTransport,
		InputTransportFactory(inputTransportFactory),
		OutputTransportFactory(outputTransportFactory),
		InputProtocolFactory(inputProtocolFactory),
		OutputProtocolFactory(outputProtocolFactory),
	)
}

// NewSimpleServerFactory create a new server factory
func NewSimpleServerFactory(processorFactory ProcessorFactory, serverTransport ServerTransport, options ...func(*SimpleServer)) *SimpleServer {
	server := &SimpleServer{
		processorFactory:       processorFactory,
		serverTransport:        serverTransport,
		inputTransportFactory:  NewTransportFactory(),
		outputTransportFactory: NewTransportFactory(),
		inputProtocolFactory:   NewBinaryProtocolFactoryDefault(),
		outputProtocolFactory:  NewBinaryProtocolFactoryDefault(),
		quit: make(chan struct{}, 1),
		log:  &log.Logger{},
	}

	for _, option := range options {
		option(server)
	}

	return server
}

// NewSimpleServerFactory2 is deprecated, used NewSimpleServerFactory instead
func NewSimpleServerFactory2(processorFactory ProcessorFactory, serverTransport ServerTransport) *SimpleServer {
	return NewSimpleServerFactory(processorFactory, serverTransport)
}

// NewSimpleServerFactory4 is deprecated, used NewSimpleServerFactory instead
func NewSimpleServerFactory4(processorFactory ProcessorFactory, serverTransport ServerTransport, transportFactory TransportFactory, protocolFactory ProtocolFactory) *SimpleServer {
	return NewSimpleServerFactory(
		processorFactory,
		serverTransport,
		TransportFactories(transportFactory),
		ProtocolFactories(protocolFactory),
	)
}

// NewSimpleServerFactory6 is deprecated, used NewSimpleServerFactory instead
func NewSimpleServerFactory6(processorFactory ProcessorFactory, serverTransport ServerTransport, inputTransportFactory TransportFactory, outputTransportFactory TransportFactory, inputProtocolFactory ProtocolFactory, outputProtocolFactory ProtocolFactory) *SimpleServer {
	return NewSimpleServerFactory(
		processorFactory,
		serverTransport,
		InputTransportFactory(inputTransportFactory),
		OutputTransportFactory(outputTransportFactory),
		InputProtocolFactory(inputProtocolFactory),
		OutputProtocolFactory(outputProtocolFactory),
	)
}

// ProcessorFactory returns the processor factory
func (p *SimpleServer) ProcessorFactory() ProcessorFactory {
	return p.processorFactory
}

// ServerTransport returns the server transport
func (p *SimpleServer) ServerTransport() ServerTransport {
	return p.serverTransport
}

// InputTransportFactory returns the input transport factory
func (p *SimpleServer) InputTransportFactory() TransportFactory {
	return p.inputTransportFactory
}

// OutputTransportFactory retunrs the output transport factory
func (p *SimpleServer) OutputTransportFactory() TransportFactory {
	return p.outputTransportFactory
}

// InputProtocolFactory retuns the input protocolfactory
func (p *SimpleServer) InputProtocolFactory() ProtocolFactory {
	return p.inputProtocolFactory
}

// OutputProtocolFactory returns the output protocol factory
func (p *SimpleServer) OutputProtocolFactory() ProtocolFactory {
	return p.outputProtocolFactory
}

// Listen returns the server transport listener
func (p *SimpleServer) Listen() error {
	return p.serverTransport.Listen()
}

// AcceptLoop runs the accept loop to handle requests
func (p *SimpleServer) AcceptLoop() error {
	for {
		client, err := p.serverTransport.Accept()
		if err != nil {
			select {
			case <-p.quit:
				return ErrServerClosed
			default:
			}
			return err
		}
		if client != nil {
			go func() {
				if err := p.processRequests(client); err != nil {
					p.log.Println("error processing request:", err)
				}
			}()
		}
	}
}

// Serve starts serving requests
func (p *SimpleServer) Serve() error {
	err := p.Listen()
	if err != nil {
		return err
	}
	return p.AcceptLoop()
}

// ServeContext starts serving requests and uses a context to cancel
func (p *SimpleServer) ServeContext(ctx context.Context) error {
	go func() {
		<-ctx.Done()
		p.Stop()
	}()
	err := p.Serve()
	if ctx.Err() != nil {
		return ctx.Err()
	}
	return err
}

// Stop stops the server
func (p *SimpleServer) Stop() error {
	p.quit <- struct{}{}
	p.serverTransport.Interrupt()
	return nil
}

func (p *SimpleServer) processRequests(client Transport) error {
	processor := p.processorFactory.Geprocessor(client)
	var (
		inputTransport, outputTransport Transport
		inputProtocol, outputProtocol   Protocol
	)

	inputTransport = p.inputTransportFactory.GetTransport(client)

	// Special case for Header, it requires that the transport/protocol for
	// input/output is the same object (to track session state).
	if _, ok := inputTransport.(*HeaderTransport); ok {
		outputTransport = nil
		inputProtocol = p.inputProtocolFactory.GetProtocol(inputTransport)
		outputProtocol = inputProtocol
	} else {
		outputTransport = p.outputTransportFactory.GetTransport(client)
		inputProtocol = p.inputProtocolFactory.GetProtocol(inputTransport)
		outputProtocol = p.outputProtocolFactory.GetProtocol(outputTransport)
	}

	defer func() {
		if err := recover(); err != nil {
			p.log.Printf("panic in processor: %v: %s", err, debug.Stack())
		}
	}()
	if inputTransport != nil {
		defer inputTransport.Close()
	}
	if outputTransport != nil {
		defer outputTransport.Close()
	}
	for {
		ok, err := processor.Process(inputProtocol, outputProtocol)
		if err, ok := err.(TransportException); ok && err.TypeId() == END_OF_FILE {
			return nil
		} else if err != nil {
			p.log.Printf("error processing request: %s", err)
			return err
		}
		if !ok {
			break
		}
	}
	return nil
}
