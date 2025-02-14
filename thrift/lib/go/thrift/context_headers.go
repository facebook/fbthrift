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

// AddHeader adds a header to the context, which will be sent as part of the request.
// AddHeader can be called multiple times to add multiple headers.
// These headers are not persistent and will only be sent with the current request.
func AddHeader(ctx context.Context, key string, value string) (context.Context, error) {
	headersMap := make(map[string]string)
	if headers := ctx.Value(types.RequestHeadersKey); headers != nil {
		var ok bool
		headersMap, ok = headers.(map[string]string)
		if !ok {
			return nil, types.NewTransportException(types.INVALID_HEADERS_TYPE, "Headers key in context value is not map[string]string")
		}
	}
	headersMap[key] = value
	ctx = context.WithValue(ctx, types.RequestHeadersKey, headersMap)
	return ctx, nil
}

// WithHeaders attaches thrift headers to a ctx.
func WithHeaders(ctx context.Context, headers map[string]string) context.Context {
	storedHeaders := ctx.Value(types.RequestHeadersKey)
	if storedHeaders == nil {
		return context.WithValue(ctx, types.RequestHeadersKey, headers)
	}
	headersMap, ok := storedHeaders.(map[string]string)
	if !ok {
		return context.WithValue(ctx, types.RequestHeadersKey, headers)
	}
	for k, v := range headers {
		headersMap[k] = v
	}
	return context.WithValue(ctx, types.RequestHeadersKey, headersMap)
}

// SetHeaders replaces all the current headers.
func SetHeaders(ctx context.Context, headers map[string]string) context.Context {
	return context.WithValue(ctx, types.RequestHeadersKey, headers)
}

// GetHeaders gets thrift headers from ctx.
func GetHeaders(ctx context.Context) map[string]string {
	// check for headersKey
	v, ok := ctx.Value(types.RequestHeadersKey).(map[string]string)
	if ok {
		return v
	}
	return nil
}
