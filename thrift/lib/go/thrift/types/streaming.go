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

package types

import (
	"runtime"
	"sync"
	"sync/atomic"
)

// ChanResult is a single stream element or terminal error, for channel-based
// consumption. A nil Err means Elem is valid. A non-nil Err is terminal: no
// more values will follow. Successful completion is signaled by channel close
// with no error result.
type ChanResult[T any] struct {
	Elem T
	Err  error
}

// StreamingHandle owns the lifetime of a single server-to-client stream.
//
// The context passed to the RPC governs the initial response only. Once the
// handle is returned, the stream is detached from that context: call Cancel to
// stop it, or let the runtime cleanup fire when the handle goes out of scope.
// Cancel is idempotent.
//
// Exactly one of Iter or Chan may be invoked per handle; invoking both (or
// either twice) panics. This is enforced with an atomic flag.
type StreamingHandle[T any] interface {
	// Iter returns the stream as an iterator. The iterator calls Cancel
	// when it returns (exhaustion, error, or early break), so a fully or
	// partially consumed iterator never leaks.
	Iter() func(yield func(T, error) bool)
	// Chan returns the underlying result channel. The caller ranges until
	// close (success) or a result with non-nil Err (terminal failure).
	// The caller owns cancellation: call Cancel when done early, or rely
	// on GC cleanup.
	Chan() <-chan ChanResult[T]
	// Cancel stops the stream. Idempotent: safe to call multiple times,
	// including via the GC cleanup and the deferred Cancel in Iter.
	Cancel()
}

// streamingHandleImpl is the StreamingHandle implementation returned to
// callers. It is unexported so all handles are built via NewStreamingHandle
// or AdaptStreamingHandle; callers only ever see the interface.
type streamingHandleImpl[T any] struct {
	ch      <-chan ChanResult[T]
	cancel  func()
	once    sync.Once
	used    atomic.Bool
	cleanup runtime.Cleanup
}

// Compile time interface enforcer.
var _ StreamingHandle[int] = (*streamingHandleImpl[int])(nil)

// NewStreamingHandle wraps a result channel with explicit cancellation.
// The producer owns closing ch; cancel must release producer resources
// (typically a context cancel func). A GC cleanup is registered so a
// forgotten handle still cancels.
func NewStreamingHandle[T any](ch <-chan ChanResult[T], cancel func()) StreamingHandle[T] {
	h := &streamingHandleImpl[T]{
		ch:     ch,
		cancel: cancel,
	}
	h.cleanup = runtime.AddCleanup(h, func(cancel func()) {
		cancel()
	}, cancel)
	return h
}

// Iter returns an iter.Seq2 over stream elements. The iterator calls Cancel
// when it returns (exhaustion, error, or early break), so a fully or partially
// consumed iterator never leaks.
func (h *streamingHandleImpl[T]) Iter() func(yield func(T, error) bool) {
	if !h.used.CompareAndSwap(false, true) {
		panic("thrift: StreamingHandle.Iter/Chan are mutually exclusive and single-use")
	}
	return func(yield func(T, error) bool) {
		defer h.Cancel()
		var zero T
		for r := range h.ch {
			if r.Err != nil {
				yield(zero, r.Err)
				return
			}
			if !yield(r.Elem, nil) {
				return
			}
		}
	}
}

// Chan returns the underlying result channel. The caller ranges until close
// (success) or a result with non-nil Err (terminal failure). The caller owns
// cancellation: call Cancel when done early, or rely on GC cleanup.
func (h *streamingHandleImpl[T]) Chan() <-chan ChanResult[T] {
	if !h.used.CompareAndSwap(false, true) {
		panic("thrift: StreamingHandle.Iter/Chan are mutually exclusive and single-use")
	}
	return h.ch
}

// Cancel stops the stream. Idempotent: safe to call multiple times, including
// via the GC cleanup and the deferred Cancel in Iter.
func (h *streamingHandleImpl[T]) Cancel() {
	h.once.Do(func() {
		h.cancel()
		h.cleanup.Stop()
	})
}

// AdaptStreamingHandle converts a handle of From elements into To elements
// for generated code (e.g. via type assertions).
func AdaptStreamingHandle[From, To any](src StreamingHandle[From], convert func(From) (To, error)) StreamingHandle[To] {
	srcCh := src.Chan()
	dstCh := make(chan ChanResult[To], DefaultStreamBufferSize)
	// done unblocks adapter sends on cancel; without it the goroutine
	// would leak once dstCh fills with no reader.
	done := make(chan struct{})
	var doneOnce sync.Once
	cancel := func() {
		doneOnce.Do(func() { close(done) })
		src.Cancel()
	}
	// send delivers one source result as (elem, err), like yield. It
	// reports false when the loop should stop: terminal result or
	// cancelled handle.
	send := func(elem From, err error) bool {
		var res ChanResult[To]
		shouldContinue := false
		if err != nil {
			res = ChanResult[To]{Err: err}
		} else {
			converted, convErr := convert(elem)
			if convErr != nil {
				res = ChanResult[To]{Err: convErr}
			} else {
				res = ChanResult[To]{Elem: converted}
				shouldContinue = true
			}
		}
		select {
		case dstCh <- res:
			return shouldContinue
		case <-done:
			return false
		}
	}
	go func() {
		defer close(dstCh)
		// Release the source when the adapter exits; idempotent.
		defer cancel()
		for r := range srcCh {
			if !send(r.Elem, r.Err) {
				return
			}
		}
	}()
	return NewStreamingHandle[To](dstCh, cancel)
}
