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
	"errors"
	"runtime"
	"testing"
	"time"

	"github.com/stretchr/testify/require"
)

func testChanResultChannel[T any](results ...ChanResult[T]) <-chan ChanResult[T] {
	ch := make(chan ChanResult[T], len(results))
	for _, r := range results {
		ch <- r
	}
	close(ch)
	return ch
}

func TestStreamingHandleIter(t *testing.T) {
	ch := testChanResultChannel(
		ChanResult[int]{Elem: 1},
		ChanResult[int]{Elem: 2},
		ChanResult[int]{Elem: 3},
	)
	cancelled := false
	h := NewStreamingHandle(ch, func() { cancelled = true })

	var got []int
	for elem, err := range h.Iter() {
		require.NoError(t, err)
		got = append(got, elem)
	}
	require.Equal(t, []int{1, 2, 3}, got)
	require.True(t, cancelled, "Iter should Cancel on exhaustion")
	// Explicit Cancel after Iter is idempotent.
	h.Cancel()
	require.True(t, cancelled)
}

func TestStreamingHandleIterError(t *testing.T) {
	sentinel := errors.New("boom")
	ch := testChanResultChannel(
		ChanResult[int]{Elem: 1},
		ChanResult[int]{Err: sentinel},
	)
	h := NewStreamingHandle(ch, func() {})

	var got []int
	var gotErr error
	for elem, err := range h.Iter() {
		if err != nil {
			gotErr = err
			break
		}
		got = append(got, elem)
	}
	require.Equal(t, []int{1}, got)
	require.ErrorIs(t, gotErr, sentinel)
}

func TestStreamingHandleChan(t *testing.T) {
	ch := testChanResultChannel(
		ChanResult[string]{Elem: "a"},
		ChanResult[string]{Elem: "b"},
	)
	cancelled := false
	h := NewStreamingHandle(ch, func() { cancelled = true })
	defer h.Cancel()

	var got []string
	for r := range h.Chan() {
		require.NoError(t, r.Err)
		got = append(got, r.Elem)
	}
	require.Equal(t, []string{"a", "b"}, got)
	require.False(t, cancelled, "Chan should not auto-cancel; caller owns Cancel")
}

func TestStreamingHandleMutualExclusion(t *testing.T) {
	newHandle := func() StreamingHandle[int] {
		return NewStreamingHandle(testChanResultChannel[int](), func() {})
	}

	h := newHandle()
	_ = h.Iter()
	require.Panics(t, func() { _ = h.Chan() })
	require.Panics(t, func() { _ = h.Iter() })

	h2 := newHandle()
	_ = h2.Chan()
	require.Panics(t, func() { _ = h2.Iter() })
	require.Panics(t, func() { _ = h2.Chan() })
}

func TestStreamingHandleCancelIdempotent(t *testing.T) {
	calls := 0
	h := NewStreamingHandle(testChanResultChannel[int](), func() { calls++ })
	h.Cancel()
	h.Cancel()
	require.Equal(t, 1, calls)
}

func TestAdaptStreamingHandle(t *testing.T) {
	src := NewStreamingHandle(testChanResultChannel(
		ChanResult[int]{Elem: 1},
		ChanResult[int]{Elem: 2},
	), func() {})
	adapted := AdaptStreamingHandle(src, func(v int) (string, error) {
		if v == 2 {
			return "", errors.New("bad value")
		}
		return "ok", nil
	})
	defer adapted.Cancel()

	var got []string
	var gotErr error
	for elem, err := range adapted.Iter() {
		if err != nil {
			gotErr = err
			break
		}
		got = append(got, elem)
	}
	require.Equal(t, []string{"ok"}, got)
	require.ErrorContains(t, gotErr, "bad value")
}

func TestAdaptStreamingHandleCancelDoesNotLeak(t *testing.T) {
	// Settle the runtime and take a goroutine baseline. Each adapter below
	// is cancelled immediately without reading: Cancel unblocks the
	// adapter directly, so no GC is needed for the count to recover. A
	// parked adapter (the pre-fix behavior) would leave +1 goroutine each,
	// well above the slack.
	runtime.GC()
	time.Sleep(100 * time.Millisecond)
	baseline := runtime.NumGoroutine()

	const adapters = 20
	for range adapters {
		// Far more elements than dstCh's buffer: a parked adapter blocks
		// on send and never observes srcCh closure.
		const n = 2000
		results := make([]ChanResult[int], 0, n)
		for j := range n {
			results = append(results, ChanResult[int]{Elem: j})
		}
		src := NewStreamingHandle(testChanResultChannel(results...), func() {})
		adapted := AdaptStreamingHandle(src, func(v int) (string, error) {
			return "ok", nil
		})
		adapted.Cancel()
	}

	require.Eventually(t, func() bool {
		return runtime.NumGoroutine() <= baseline+5
	}, 10*time.Second, 20*time.Millisecond, "adapter goroutines did not exit after Cancel")
}
