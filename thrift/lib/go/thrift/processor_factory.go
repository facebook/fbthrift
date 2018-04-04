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

// The default processor factory just returns a singleton
// instance.
type ProcessorFactory interface {
	Geprocessor(trans Transport) Processor
}

type processorFactory struct {
	processor Processor
}

func NewProcessorFactory(p Processor) ProcessorFactory {
	return &processorFactory{processor: p}
}

func (p *processorFactory) Geprocessor(trans Transport) Processor {
	return p.processor
}

/**
 * The default processor factory just returns a singleton
 * instance.
 */
type ProcessorFunctionFactory interface {
	GetProcessorFunction(trans Transport) ProcessorFunction
}

type processorFunctionFactory struct {
	processor ProcessorFunction
}

func NewProcessorFunctionFactory(p ProcessorFunction) ProcessorFunctionFactory {
	return &processorFunctionFactory{processor: p}
}

func (p *processorFunctionFactory) GetProcessorFunction(trans Transport) ProcessorFunction {
	return p.processor
}

// ConcurrentProcessorFactory is the concurrent counterpart of ProcessorFactory
type ConcurrentProcessorFactory interface {
	Geprocessor(trans Transport) ConcurrentProcessor
}

type concurrentProcessorFactory struct {
	processor ConcurrentProcessor
}

// NewConcurrentProcessorFactory returns a factory wrapping a ConcurrentProcessor
func NewConcurrentProcessorFactory(p ConcurrentProcessor) ConcurrentProcessorFactory {
	return &concurrentProcessorFactory{processor: p}
}

func (p *concurrentProcessorFactory) Geprocessor(trans Transport) ConcurrentProcessor {
	return p.processor
}

// ConcurrentProcessorFunctionFactory is the concurrent counterpat of ProcessorFunctionFactory
type ConcurrentProcessorFunctionFactory interface {
	GetConcurrentProcessorFunction(trans Transport) ConcurrentProcessorFunction
}

type concurrentProcessorFunctionFactory struct {
	processor ConcurrentProcessorFunction
}

// NewConcurrentProcessorFunctionFactory creates a new factory wrapping a
// ConcurrentProcessorFunction
func NewConcurrentProcessorFunctionFactory(p ConcurrentProcessorFunction) ConcurrentProcessorFunctionFactory {
	return &concurrentProcessorFunctionFactory{processor: p}
}

func (p *concurrentProcessorFunctionFactory) GetConcurrentProcessorFunction(trans Transport) ConcurrentProcessorFunction {
	return p.processor
}
