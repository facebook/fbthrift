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

use crate::framed_transport::FramedTransport;
use bytes::Bytes;
use futures::{stream, Stream};
use std::io::{self, Cursor};
use tokio::runtime::Runtime;
use tokio_proto::pipeline::ClientProto;

#[test]
fn framed_transport_encode() {
    let buf = Cursor::new(Vec::with_capacity(32));

    let trans = FramedTransport.bind_transport(buf).unwrap();

    let input = Bytes::from(vec![0u8, 1, 2, 3, 4, 5, 6, 7]);

    let stream = stream::once::<_, io::Error>(Ok(input));

    let fut = stream.forward(trans);

    let mut runtime = Runtime::new().unwrap();

    let (_stream, trans) = runtime.block_on(fut).unwrap();

    let expected = vec![0, 0, 0, 8, 0, 1, 2, 3, 4, 5, 6, 7];

    let encoded = trans.into_inner().into_inner();

    assert_eq!(encoded, expected, "encoded frame not equal");
}

#[test]
fn framed_transport_decode() {
    let buf = Cursor::new(vec![0u8, 0, 0, 8, 0, 1, 2, 3, 4, 5, 6, 7]);

    let trans = FramedTransport.bind_transport(buf).unwrap();

    let fut = trans.collect();

    let mut runtime = Runtime::new().unwrap();

    let mut decoded = runtime.block_on(fut).unwrap();

    let decoded = decoded.pop().unwrap();

    let expected = vec![0u8, 1, 2, 3, 4, 5, 6, 7];

    assert_eq!(decoded.into_inner(), expected, "decoded frame not equal");
}

#[test]
fn framed_transport_decode_incomplete_frame() {
    // Promise 8, deliver 7
    let buf = Cursor::new(vec![0u8, 0, 0, 8, 0, 1, 2, 3, 4, 5, 6]);

    let trans = FramedTransport.bind_transport(buf).unwrap();

    let fut = trans.collect();

    let mut runtime = Runtime::new().unwrap();

    assert!(
        runtime.block_on(fut).is_err(),
        "returned Ok with bytes left on stream"
    );
}

#[test]
fn framed_transport_decode_incomplete_header() {
    // Promise 8, deliver 7
    let buf = Cursor::new(vec![0u8, 0, 0]);

    let trans = FramedTransport.bind_transport(buf).unwrap();

    let fut = trans.collect();

    let mut runtime = Runtime::new().unwrap();

    assert!(
        runtime.block_on(fut).is_err(),
        "returned Ok with bytes left on stream"
    );
}
