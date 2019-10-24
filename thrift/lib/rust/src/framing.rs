/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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

use crate::bufext::{BufExt, BufMutExt};
use bytes::{Bytes, BytesMut};
use std::io::Cursor;

/// Helper type alias to get encoding buffer type
pub type FramingEncoded<F> = <F as Framing>::EncBuf;

/// Helper type alias to get the type of the finalized encoded buffer
pub type FramingEncodedFinal<F> = <<F as Framing>::EncBuf as BufMutExt>::Final;

/// Helper type alias to get the buffer to use as input to decoding
pub type FramingDecoded<F> = <F as Framing>::DecBuf;

/// Trait describing the in-memory frames the transport uses for Protocol messages.
pub trait Framing {
    /// Buffer type we encode into
    type EncBuf: BufMutExt + Send + 'static;

    /// Buffer type we decode from
    type DecBuf: BufExt + Send + 'static;

    /// Arbitrary metadata associated with a frame. This is passed to server methods, but
    /// they need to specify some particular concrete type (ie, be implemented for a specific
    /// kind of framing) in order to make use of it.
    type Meta;

    /// Allocate a new encoding buffer with a given capacity
    /// FIXME: need &self?
    fn enc_with_capacity(cap: usize) -> Self::EncBuf;

    /// Get the metadata associated with this frame.
    fn get_meta(&self) -> Self::Meta;
}

impl Framing for Bytes {
    type EncBuf = BytesMut;
    type DecBuf = Cursor<Bytes>;
    type Meta = ();

    fn enc_with_capacity(cap: usize) -> Self::EncBuf {
        BytesMut::with_capacity(cap)
    }

    fn get_meta(&self) {}
}
