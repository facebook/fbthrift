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
	"github.com/facebook/fbthrift/thrift/lib/go/thrift/types"
)

// Protocol defines the interface that must be implemented by all protocols
type Protocol interface {
	Format

	// used by SerialChannel and generated thrift Clients
	Close() error

	// Deprecated
	setRequestHeader(key, value string)
	// Deprecated
	getResponseHeaders() map[string]string
	// Deprecated
	DO_NOT_USE_GetResponseHeaders() map[string]string

	types.DO_NOT_USE_ChannelWrapper
}
