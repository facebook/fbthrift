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

use crate::ApplicationException;
use failure::{Error, Fail};

#[derive(Debug, Fail)]
#[allow(dead_code)]
pub enum ProtocolError {
    #[fail(display = "end of file reached")]
    EOF,
    #[fail(display = "bad thrift version specified")]
    BadVersion,
    #[fail(display = "missing protocol version")]
    ProtocolVersionMissing,
    #[fail(display = "protocol skip depth exceeded")]
    SkipDepthExceeded,
    #[fail(display = "streams unsupported")]
    StreamUnsupported,
    #[fail(display = "STOP outside of struct in skip")]
    UnexpectedStopInSkip,
    #[fail(display = "Unknown or invalid protocol ID {}", _0)]
    InvalidProtocolID(i16),
    #[fail(display = "Unknown or invalid TMessage type {}", _0)]
    InvalidMessageType(u32),
    #[fail(display = "Unknown or invalid type tag")]
    InvalidTypeTag,
    #[fail(display = "Unknown or invalid data length")]
    InvalidDataLength,
    #[fail(display = "Invalid value for type")]
    InvalidValue,
    #[fail(display = "Application exception: {:?}", _0)]
    ApplicationException(ApplicationException),
}

impl From<ApplicationException> for Error {
    fn from(exn: ApplicationException) -> Error {
        ProtocolError::ApplicationException(exn).into()
    }
}
