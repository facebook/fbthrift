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

#![recursion_limit = "1024"]
#![deny(warnings)]

use std::i32;

macro_rules! bail_err {
    ($e:expr) => {
        return Err(From::from($e));
    };
}

macro_rules! ensure_err {
    ($cond:expr, $e:expr) => {
        if !$cond {
            bail_err!($e);
        }
    };
}

use anyhow::Result;

#[macro_use]
pub mod protocol;

pub mod application_exception;
pub mod binary_protocol;
pub mod compact_protocol;
pub mod deserialize;
pub mod export;
pub mod framing;
pub mod processor;
pub mod serialize;
pub mod thrift_protocol;
pub mod ttype;

mod bufext;
mod client;
mod errors;
mod varint;

#[cfg(test)]
mod tests;

pub mod types {
    // Define ApplicationException as if it were a normal generated type to make things simpler
    // for codegen.
    pub use crate::application_exception::ApplicationException;
}

pub use crate::application_exception::{ApplicationException, ApplicationExceptionErrorCode};
pub use crate::binary_protocol::BinaryProtocol;
pub use crate::bufext::{BufExt, BufMutExt};
pub use crate::client::{ClientFactory, Transport};
pub use crate::compact_protocol::CompactProtocol;
pub use crate::deserialize::Deserialize;
pub use crate::errors::{NonthrowingFunctionError, ProtocolError};
pub use crate::framing::{Framing, FramingDecoded, FramingEncoded, FramingEncodedFinal};
pub use crate::processor::{NullServiceProcessor, ServiceProcessor, ThriftService};
pub use crate::protocol::{
    Protocol, ProtocolDecoded, ProtocolEncoded, ProtocolEncodedFinal, ProtocolReader,
    ProtocolWriter,
};
pub use crate::serialize::Serialize;
pub use crate::thrift_protocol::{MessageType, ProtocolID};
pub use crate::ttype::{GetTType, TType};

/// Set the default ID's for unknown exceptions and fields.
/// When reading off the wire, these default values will be
/// overridden with the unrecognized id (which must be nonnegative).
// ---
// Keep in sync with the UNKNOWN_ID constant in //common/rust/thrift/ast.
pub const __UNKNOWN_ID: i32 = i32::MIN;
