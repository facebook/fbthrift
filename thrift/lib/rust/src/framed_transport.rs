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

use byteorder::{BigEndian, ByteOrder};
use bytes::{BufMut, Bytes, BytesMut};
use std::io::{self, Cursor};
use tokio_codec::{Decoder, Encoder, Framed};
use tokio_io::{AsyncRead, AsyncWrite};
use tokio_proto::pipeline::{ClientProto, ServerProto};

pub struct FramedTransport;
impl<T> ClientProto<T> for FramedTransport
where
    T: AsyncRead + AsyncWrite + 'static,
{
    type Request = Bytes;
    type Response = Cursor<Bytes>;
    type Transport = Framed<T, FramedTransportCodec>;
    type BindTransport = Result<Self::Transport, io::Error>;

    fn bind_transport(&self, io: T) -> Self::BindTransport {
        Ok(FramedTransportCodec.framed(io))
    }
}

impl<T> ServerProto<T> for FramedTransport
where
    T: AsyncRead + AsyncWrite + 'static,
{
    type Request = Cursor<Bytes>;
    type Response = Bytes;
    type Transport = Framed<T, FramedTransportCodec>;
    type BindTransport = Result<Self::Transport, io::Error>;

    fn bind_transport(&self, io: T) -> Self::BindTransport {
        Ok(FramedTransportCodec.framed(io))
    }
}

pub struct FramedTransportCodec;
impl Encoder for FramedTransportCodec {
    type Item = Bytes;
    type Error = io::Error;

    fn encode(&mut self, item: Self::Item, dst: &mut BytesMut) -> Result<(), Self::Error> {
        dst.reserve(4 + item.len());
        dst.put_u32_be(item.len() as u32);
        dst.put(item);
        Ok(())
    }
}

impl Decoder for FramedTransportCodec {
    type Item = Cursor<Bytes>;
    type Error = io::Error;

    fn decode(&mut self, src: &mut BytesMut) -> Result<Option<Self::Item>, Self::Error> {
        // Wait for at least a frame header
        if src.len() < 4 {
            return Ok(None);
        }

        // Peek at first 4 bytes, don't advance src buffer
        let len = BigEndian::read_u32(&src[..4]) as usize;

        // Make sure we have all the bytes we were promised
        if src.len() < 4 + len {
            return Ok(None);
        }

        // Drain 4 bytes from src
        let _ = src.split_to(4).as_ref();

        // Take len bytes, advancing src
        let res = src.split_to(len);

        Ok(Some(Cursor::new(res.freeze())))
    }
}
