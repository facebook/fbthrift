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
	"log"
	"runtime/debug"
	"sync"
)

// ConcurrentServer is the concurrent counterpart of SimpleServer
// It is able to process out-of-order requests on the same transport
type ConcurrentServer struct {
	processorFactory ConcurrentProcessorFactory
	*ServerOptions
}

// NewConcurrentServer create a new NewConcurrentServer
func NewConcurrentServer(processor ConcurrentProcessor, serverTransport ServerTransport, options ...func(*ServerOptions)) *ConcurrentServer {
	return NewConcurrentServerFactory(NewConcurrentProcessorFactory(processor), serverTransport, options...)
}

// NewConcurrentServer2 is deprecated, use NewConcurrentServer
func NewConcurrentServer2(processor ConcurrentProcessor, serverTransport ServerTransport) *ConcurrentServer {
	return NewConcurrentServerFactory(NewConcurrentProcessorFactory(processor), serverTransport)
}

// NewConcurrentServer4 is deprecated, use NewConcurrentServer
func NewConcurrentServer4(processor ConcurrentProcessor, serverTransport ServerTransport, transportFactory TransportFactory, protocolFactory ProtocolFactory) *ConcurrentServer {
	return NewConcurrentServerFactory(NewConcurrentProcessorFactory(processor),
		serverTransport,
		TransportFactories(transportFactory),
		ProtocolFactories(protocolFactory),
	)
}

// NewConcurrentServer6 is deprecated, use NewConcurrentServer
func NewConcurrentServer6(processor ConcurrentProcessor, serverTransport ServerTransport,
	inputTransportFactory TransportFactory, outputTransportFactory TransportFactory,
	inputProtocolFactory ProtocolFactory, outputProtocolFactory ProtocolFactory) *ConcurrentServer {
	return NewConcurrentServerFactory(NewConcurrentProcessorFactory(processor),
		serverTransport,
		InputTransportFactory(inputTransportFactory),
		OutputTransportFactory(outputTransportFactory),
		InputProtocolFactory(inputProtocolFactory),
		OutputProtocolFactory(outputProtocolFactory),
	)
}

// NewConcurrentServerFactory create a new server factory
func NewConcurrentServerFactory(processorFactory ConcurrentProcessorFactory, serverTransport ServerTransport, options ...func(*ServerOptions)) *ConcurrentServer {
	serverOptions := defaultServerOptions(serverTransport)

	for _, option := range options {
		option(serverOptions)
	}

	return &ConcurrentServer{processorFactory, serverOptions}
}

// NewConcurrentServerFactory2 is deprecated, use NewConcurrentServerFactory
func NewConcurrentServerFactory2(processorFactory ConcurrentProcessorFactory, serverTransport ServerTransport) *ConcurrentServer {
	return NewConcurrentServerFactory(processorFactory, serverTransport)
}

// NewConcurrentServerFactory4 is deprecated, use NewConcurrentServerFactory
func NewConcurrentServerFactory4(processorFactory ConcurrentProcessorFactory, serverTransport ServerTransport,
	transportFactory TransportFactory, protocolFactory ProtocolFactory) *ConcurrentServer {
	return NewConcurrentServerFactory(processorFactory, serverTransport, TransportFactories(transportFactory), ProtocolFactories(protocolFactory))
}

// NewConcurrentServerFactory6 is deprecated, use NewConcurrentServerFactory
func NewConcurrentServerFactory6(processorFactory ConcurrentProcessorFactory, serverTransport ServerTransport,
	inputTransportFactory TransportFactory, outputTransportFactory TransportFactory,
	inputProtocolFactory ProtocolFactory, outputProtocolFactory ProtocolFactory) *ConcurrentServer {
	return NewConcurrentServerFactory(
		processorFactory,
		serverTransport,
		InputTransportFactory(inputTransportFactory),
		OutputTransportFactory(outputTransportFactory),
		InputProtocolFactory(inputProtocolFactory),
		OutputProtocolFactory(outputProtocolFactory),
	)
}

// ConcurrentProcessorFactory returns the processor factory of the server
func (p *ConcurrentServer) ConcurrentProcessorFactory() ConcurrentProcessorFactory {
	return p.processorFactory
}

// ServerTransport returns the transport of the server
func (p *ConcurrentServer) ServerTransport() ServerTransport {
	return p.serverTransport
}

// InputTransportFactory returns the input transport of the server
func (p *ConcurrentServer) InputTransportFactory() TransportFactory {
	return p.inputTransportFactory
}

// OutputTransportFactory returns the output transport of the server
func (p *ConcurrentServer) OutputTransportFactory() TransportFactory {
	return p.outputTransportFactory
}

// InputProtocolFactory returns the input protocol factory of the server
func (p *ConcurrentServer) InputProtocolFactory() ProtocolFactory {
	return p.inputProtocolFactory
}

// OutputProtocolFactory returns the output protocol factory of the server
func (p *ConcurrentServer) OutputProtocolFactory() ProtocolFactory {
	return p.outputProtocolFactory
}

// Listen starts listening on the transport
func (p *ConcurrentServer) Listen() error {
	return p.serverTransport.Listen()
}

// AcceptLoop starts accepting connections from the transport
// This loops forever until Stop() is called or on error
func (p *ConcurrentServer) AcceptLoop() error {
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
					log.Println("error processing request:", err)
				}
			}()
		}
	}
}

// Serve starts listening on the transport and accepting new connections
// This loops forever until Stop() is called or on error
func (p *ConcurrentServer) Serve() error {
	err := p.Listen()
	if err != nil {
		return err
	}
	return p.AcceptLoop()
}

// ServeContext is Serve that can be used with the standard library's Context
func (p *ConcurrentServer) ServeContext(ctx context.Context) error {
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

// Stop stops the accept loop
// This will block if the accept loop is not yet started
func (p *ConcurrentServer) Stop() error {
	p.quit <- struct{}{}
	p.serverTransport.Interrupt()
	return nil
}

func (p *ConcurrentServer) processRequests(client Transport) error {
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

	// recover from any panic in the processor, so it doesn't crash the
	// thrift server
	defer func() {
		if e := recover(); e != nil {
			log.Printf("panic in processor: %s: %s", e, debug.Stack())
		}
	}()
	if inputTransport != nil {
		defer inputTransport.Close()
	}
	if outputTransport != nil {
		defer outputTransport.Close()
	}

	mut := sync.Mutex{}
	for {
		ok, err := processor.ProcessConcurrent(inputProtocol, outputProtocol, &mut)
		if err, ok := err.(TransportException); ok && err.TypeID() == END_OF_FILE {
			return nil
		} else if err != nil {
			log.Printf("error processing request: %s", err)
			return err
		}
		if !ok {
			break
		}
	}
	return nil
}
