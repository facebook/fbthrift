/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package thrift

import (
	"context"
	"slices"

	"github.com/facebook/fbthrift/thrift/lib/go/thrift/types"
)

// ChainInterceptors returns a thrift interceptor that chains the execution of
// the interceptors present in its arguments. Execution happens in order of
// appearance.
//
// Deprecated: Use the modern ServiceInterceptor API instead.
func ChainInterceptors(interceptors ...Interceptor) Interceptor {
	return func(
		ctx context.Context,
		name string,
		pf types.ProcessorFunction,
		args types.ReadableStruct,
	) (types.WritableResult, error) {
		if len(interceptors) == 0 {
			return pf.RunContext(ctx, args)
		}
		handler := &chainHandler{
			curI:         0,
			name:         name,
			origHandler:  pf,
			interceptors: interceptors,
		}
		return interceptors[0](ctx, name, handler, args)
	}
}

// runOnRequestInterceptors invokes every registered ServiceInterceptor's
// OnRequest in forward (registration) order, after the request is deserialized
// and before the handler runs.
//
// Each interceptor returns the context to use for the rest of the request;
// interceptors carry per-request state by storing it in that context and reading
// it back in OnResponse. To protect the chain, the returned context is accepted
// only if it is non-nil and still carries an unexported sentinel, proving it was
// derived from the one we passed in; otherwise it is discarded
// and the previous context carried forward.
//
// All interceptors always run, even if an earlier one errors; the first error is
// returned and signals that the handler must not run.
//
// userConnState is currently always nil: connection-scoped state is not yet
// supported.
func runOnRequestInterceptors(ctx context.Context, req ReadableStruct, interceptors []ServiceInterceptor) (context.Context, error) {
	if len(interceptors) == 0 {
		return ctx, nil
	}
	// interceptorContextSentinelKey marks the context we hand to OnRequest. A
	// returned context is accepted only if it still carries this sentinel,
	// proving it was derived from ours and not replaced with nil or a fresh
	// context that would drop request-scoped values.
	type interceptorContextSentinelKey struct{}
	var firstErr error
	// Tag the context so we can detect a context not derived from this one.
	ctx = context.WithValue(ctx, interceptorContextSentinelKey{}, struct{}{})
	for _, interceptor := range interceptors {
		ctxPrime, err := interceptor.OnRequest(ctx, req, nil /* userConnState */)
		if err != nil && firstErr == nil {
			firstErr = err
		}
		// Discard a nil context, or one missing our sentinel, and keep the
		// previous one.
		if ctxPrime == nil || ctxPrime.Value(interceptorContextSentinelKey{}) == nil {
			continue
		}
		ctx = ctxPrime
	}
	return ctx, firstErr
}

// runOnResponseInterceptors invokes the OnResponse callback of every registered
// ServiceInterceptor in reverse order relative to registration, so that the
// first interceptor to observe OnRequest is the last to observe OnResponse
// (matching the C++ ServiceInterceptor ordering contract). It is meant to be
// called by the server's RPC handling paths before the outgoing response is
// serialized.
//
// The ctx passed in is the one returned by runOnRequestInterceptors, so any
// per-request state an interceptor stored in the context during OnRequest can be
// read back from ctx in OnResponse.
//
// All interceptors are always invoked, even if one returns an error. When
// multiple interceptors return errors, the last one encountered is returned.
// Because iteration is in reverse, that is the error from the earliest-registered
// interceptor, matching the C++ behavior where an OnResponse exception overwrites
// any currently-active exception.
func runOnResponseInterceptors(ctx context.Context, respRes WritableResult, respErr error, interceptors []ServiceInterceptor) error {
	result := InterceptorResult{Response: respRes, Err: respErr}
	var lastErr error
	for _, interceptor := range slices.Backward(interceptors) {
		err := interceptor.OnResponse(ctx, result)
		if err != nil {
			lastErr = err
		}
	}
	return lastErr
}

// chainHandler is a utility struct that implements the ProcessorFunction
// interface and executes the interceptors in the list in order.
type chainHandler struct {
	curI         int
	name         string
	origHandler  types.ProcessorFunction
	interceptors []Interceptor
}

// NewReqArgs...
func (ch *chainHandler) NewReqArgs() types.ReadableStruct {
	return nil
}

func (ch *chainHandler) RunContext(ctx context.Context, args types.ReadableStruct) (types.WritableResult, error) {
	if ch.curI == len(ch.interceptors)-1 {
		return ch.origHandler.RunContext(ctx, args)
	}
	ch.curI++
	return ch.interceptors[ch.curI](ctx, ch.name, ch, args)
}
