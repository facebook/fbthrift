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

pub trait BinaryType: From<Vec<u8>> {
    fn with_capacity(capacity: usize) -> Self;
    fn extend_from_slice(&mut self, other: &[u8]);
}

impl BinaryType for Vec<u8> {
    fn with_capacity(capacity: usize) -> Self {
        Vec::with_capacity(capacity)
    }
    fn extend_from_slice(&mut self, other: &[u8]) {
        Vec::extend_from_slice(self, other);
    }
}

pub(crate) struct Discard;

impl From<Vec<u8>> for Discard {
    fn from(_v: Vec<u8>) -> Self {
        Discard
    }
}

impl BinaryType for Discard {
    fn with_capacity(_capacity: usize) -> Self {
        Discard
    }
    fn extend_from_slice(&mut self, _other: &[u8]) {}
}
