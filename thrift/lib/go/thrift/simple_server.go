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

// Simple, non-concurrent server for testing.
type SimpleServer struct {
	quit chan struct{}

	processorFactory       ProcessorFactory
	serverTransport        ServerTransport
	inputTransportFactory  TransportFactory
	outputTransportFactory TransportFactory
	inpuprotocolFactory   ProtocolFactory
	outpuprotocolFactory  ProtocolFactory
}

func NewSimpleServer2(processor Processor, serverTransport ServerTransport) *SimpleServer {
	return NewSimpleServerFactory2(NewProcessorFactory(processor), serverTransport)
}

func NewSimpleServer4(processor Processor, serverTransport ServerTransport, transportFactory TransportFactory, protocolFactory ProtocolFactory) *SimpleServer {
	return NewSimpleServerFactory4(NewProcessorFactory(processor),
		serverTransport,
		transportFactory,
		protocolFactory,
	)
}

func NewSimpleServer6(processor Processor, serverTransport ServerTransport, inputTransportFactory TransportFactory, outputTransportFactory TransportFactory, inpuprotocolFactory ProtocolFactory, outpuprotocolFactory ProtocolFactory) *SimpleServer {
	return NewSimpleServerFactory6(NewProcessorFactory(processor),
		serverTransport,
		inputTransportFactory,
		outputTransportFactory,
		inpuprotocolFactory,
		outpuprotocolFactory,
	)
}

func NewSimpleServerFactory2(processorFactory ProcessorFactory, serverTransport ServerTransport) *SimpleServer {
	return NewSimpleServerFactory6(processorFactory,
		serverTransport,
		NewTransportFactory(),
		NewTransportFactory(),
		NewBinaryProtocolFactoryDefault(),
		NewBinaryProtocolFactoryDefault(),
	)
}

func NewSimpleServerFactory4(processorFactory ProcessorFactory, serverTransport ServerTransport, transportFactory TransportFactory, protocolFactory ProtocolFactory) *SimpleServer {
	return NewSimpleServerFactory6(processorFactory,
		serverTransport,
		transportFactory,
		transportFactory,
		protocolFactory,
		protocolFactory,
	)
}

func NewSimpleServerFactory6(processorFactory ProcessorFactory, serverTransport ServerTransport, inputTransportFactory TransportFactory, outputTransportFactory TransportFactory, inpuprotocolFactory ProtocolFactory, outpuprotocolFactory ProtocolFactory) *SimpleServer {
	return &SimpleServer{
		processorFactory:       processorFactory,
		serverTransport:        serverTransport,
		inputTransportFactory:  inputTransportFactory,
		outputTransportFactory: outputTransportFactory,
		inpuprotocolFactory:   inpuprotocolFactory,
		outpuprotocolFactory:  outpuprotocolFactory,
		quit: make(chan struct{}, 1),
	}
}

func (p *SimpleServer) ProcessorFactory() ProcessorFactory {
	return p.processorFactory
}

func (p *SimpleServer) ServerTransport() ServerTransport {
	return p.serverTransport
}

func (p *SimpleServer) InputTransportFactory() TransportFactory {
	return p.inputTransportFactory
}

func (p *SimpleServer) OutputTransportFactory() TransportFactory {
	return p.outputTransportFactory
}

func (p *SimpleServer) InpuprotocolFactory() ProtocolFactory {
	return p.inpuprotocolFactory
}

func (p *SimpleServer) OutpuprotocolFactory() ProtocolFactory {
	return p.outpuprotocolFactory
}

func (p *SimpleServer) Listen() error {
	return p.serverTransport.Listen()
}

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
					log.Println("error processing request:", err)
				}
			}()
		}
	}
}

func (p *SimpleServer) Serve() error {
	err := p.Listen()
	if err != nil {
		return err
	}
	return p.AcceptLoop()
}

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

func (p *SimpleServer) Stop() error {
	p.quit <- struct{}{}
	p.serverTransport.Interrupt()
	return nil
}

func (p *SimpleServer) processRequests(client Transport) error {
	processor := p.processorFactory.Geprocessor(client)
	var (
		inputTransport, outputTransport Transport
		inpuprotocol, outpuprotocol   Protocol
	)

	inputTransport = p.inputTransportFactory.GetTransport(client)

	// Special case for Header, it requires that the transport/protocol for
	// input/output is the same object (to track session state).
	if _, ok := inputTransport.(*HeaderTransport); ok {
		outputTransport = nil
		inpuprotocol = p.inpuprotocolFactory.GetProtocol(inputTransport)
		outpuprotocol = inpuprotocol
	} else {
		outputTransport = p.outputTransportFactory.GetTransport(client)
		inpuprotocol = p.inpuprotocolFactory.GetProtocol(inputTransport)
		outpuprotocol = p.outpuprotocolFactory.GetProtocol(outputTransport)
	}

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
	for {
		ok, err := processor.Process(inpuprotocol, outpuprotocol)
		if err, ok := err.(TransportException); ok && err.TypeId() == END_OF_FILE {
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
