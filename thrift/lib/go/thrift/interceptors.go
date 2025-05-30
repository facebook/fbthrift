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

	"github.com/facebook/fbthrift/thrift/lib/go/thrift/types"
)

// ChainInterceptors returns a thrift interceptor that chains the execution of
// the interceptors present in its arguments. Execution happens in order of
// appearance.
func ChainInterceptors(interceptors ...Interceptor) Interceptor {
	n := len(interceptors)
	switch n {
	case 0:
		return func(ctx context.Context, name string, pf types.ProcessorFunction,
			args types.Struct) (types.WritableStruct, types.ApplicationExceptionIf) {

			return pf.RunContext(ctx, args)
		}
	case 1:
		return interceptors[0]
	}

	return func(ctx context.Context, name string, pf types.ProcessorFunction,
		args types.Struct) (types.WritableStruct, types.ApplicationExceptionIf) {

		handler := &chainHandler{
			last:         n - 1,
			name:         name,
			origHandler:  pf,
			interceptors: interceptors,
		}
		return interceptors[0](ctx, name, handler, args)
	}
}

// chainHandler is a utility struct that implements the ProcessorFunction
// interface and executes the interceptors in the list in order.
type chainHandler struct {
	curI         int
	last         int
	name         string
	origHandler  types.ProcessorFunction
	interceptors []Interceptor
}

// Read does nothing here, it is not used and shouldn't be called
func (ch *chainHandler) Read(_ types.Decoder) (types.Struct, error) {
	return nil, nil
}

// Write does nothing here, it is not used and shouldn't be called
func (ch *chainHandler) Write(_ int32, _ types.WritableStruct, _ types.Encoder) error {
	return nil
}

func (ch *chainHandler) RunContext(ctx context.Context, args types.Struct) (types.WritableStruct, types.ApplicationExceptionIf) {
	if ch.curI == ch.last {
		return ch.origHandler.RunContext(ctx, args)
	}
	ch.curI++
	return ch.interceptors[ch.curI](ctx, ch.name, ch, args)
}
