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
	"fmt"
	"log"
	"runtime/debug"
	"sync"
)

// ConcurrentServer is the concurrent counterpart of SimpleServer
// It is able to process out-of-order requests on the same transport
type ConcurrentServer struct {
	*SimpleServer
}

// NewConcurrentServer create a new NewConcurrentServer
func NewConcurrentServer(processor Processor, serverTransport ServerTransport, options ...func(*ServerOptions)) *ConcurrentServer {
	return NewConcurrentServerFactory(NewProcessorFactory(processor), serverTransport, options...)
}

// NewConcurrentServerFactory create a new server factory
func NewConcurrentServerFactory(processorFactory ProcessorFactory, serverTransport ServerTransport, options ...func(*ServerOptions)) *ConcurrentServer {
	serverOptions := defaultServerOptions(serverTransport)

	for _, option := range options {
		option(serverOptions)
	}

	return &ConcurrentServer{&SimpleServer{processorFactory, serverOptions}}
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

	// WARNING: This server implementation has a host of problems, and is included
	// to preserve previous behavior.  If you really want a production quality thrift
	// server, use simple server or write your own.
	//
	// In the concurrent server case, we wish to handle multiple concurrent requests
	// on a single transport.  To do this, we re-implement the generated Process()
	// function inline for greater control, then directly interact with the Read(),
	// Run(), and Write() functionality.
	//
	// Note, for a very high performance server, it is unclear that this unbounded
	// concurrency is productive for maintaining maximal throughput with good
	// characteristics under load.
	var writeLock sync.Mutex
	for {
		name, _, seqID, err := inputProtocol.ReadMessageBegin()
		if err != nil {
			if err, ok := err.(TransportException); ok && err.TypeID() == END_OF_FILE {
				// connection terminated because client closed connection
				break
			}
			return err
		}
		pfunc, err := processor.GetProcessorFunction(name)
		if pfunc == nil || err != nil {
			if err == nil {
				err = fmt.Errorf("no such function: %q", name)
			}
			inputProtocol.Skip(STRUCT)
			inputProtocol.ReadMessageEnd()
			exc := NewApplicationException(UNKNOWN_METHOD, err.Error())

			// protect message writing
			writeLock.Lock()
			defer writeLock.Unlock()

			outputProtocol.WriteMessageBegin(name, EXCEPTION, seqID)
			exc.Write(outputProtocol)
			outputProtocol.WriteMessageEnd()
			outputProtocol.Flush()
			return exc
		}
		argStruct, err := pfunc.Read(inputProtocol)
		if err != nil {
			return err
		}
		go func() {
			var result WritableStruct
			result, err = pfunc.Run(argStruct)
			// protect message writing
			writeLock.Lock()
			defer writeLock.Unlock()
			if err != nil && result == nil {
				// if the Run function generates an error, synthesize an application
				// error
				exc := NewApplicationException(INTERNAL_ERROR, "Internal error: "+err.Error())
				err, result = exc, exc
			}
			pfunc.Write(seqID, result, outputProtocol)
			// ignore write failures explicitly.  This emulates previous behavior
			// we hope that the read will fail and the connection will be closed
			// well.
		}()
	}
	// graceful exit
	return nil
}
