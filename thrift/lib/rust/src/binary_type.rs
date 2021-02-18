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

use crate::bufext::BufExt;
use bytes::Bytes;

/// Trait implemented on types that can be used as `binary` types in
/// thrift.  These types copy data from the Thrift buffer.
pub trait BinaryType {
    fn with_capacity(capacity: usize) -> Self;
    fn extend_from_slice(&mut self, other: &[u8]);
}

/// Trait for copying from the Thrift buffer.  Special implementations
/// may do this without actually copying.
pub trait CopyFromBuf {
    fn copy_from_buf<B: BufExt>(buffer: &mut B, len: usize) -> Self;
}

impl BinaryType for Vec<u8> {
    fn with_capacity(capacity: usize) -> Self {
        Vec::with_capacity(capacity)
    }
    fn extend_from_slice(&mut self, other: &[u8]) {
        Vec::extend_from_slice(self, other);
    }
}

impl<T: BinaryType> CopyFromBuf for T {
    fn copy_from_buf<B: BufExt>(buffer: &mut B, len: usize) -> Self {
        assert!(buffer.remaining() >= len);
        let mut result = T::with_capacity(len);
        let mut remaining = len;

        while remaining > 0 {
            let part = buffer.bytes();
            let part_len = part.len().min(remaining);
            result.extend_from_slice(&part[..part_len]);
            remaining -= part_len;
            buffer.advance(part_len);
        }

        result
    }
}

pub(crate) struct Discard;

impl From<Vec<u8>> for Discard {
    fn from(_v: Vec<u8>) -> Discard {
        Discard
    }
}

impl CopyFromBuf for Discard {
    fn copy_from_buf<B: BufExt>(buffer: &mut B, len: usize) -> Self {
        buffer.advance(len);
        Discard
    }
}

impl CopyFromBuf for Bytes {
    fn copy_from_buf<B: BufExt>(buffer: &mut B, len: usize) -> Self {
        buffer.copy_to_bytes(len)
    }
}
