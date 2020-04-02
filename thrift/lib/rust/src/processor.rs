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

use crate::application_exception::{ApplicationException, ApplicationExceptionErrorCode};
use crate::framing::{Framing, FramingDecoded, FramingEncodedFinal};
use crate::protocol::{self, Protocol, ProtocolDecoded, ProtocolEncodedFinal, ProtocolReader};
use crate::serialize::Serialize;
use crate::thrift_protocol::MessageType;
use anyhow::Error;
use async_trait::async_trait;
use std::marker::PhantomData;

#[async_trait]
pub trait ThriftService<F>: Send + Sync + 'static
where
    F: Framing + Send + 'static,
{
    type Handler;
    type RequestContext;

    async fn call(
        &self,
        req: FramingDecoded<F>,
        req_ctxt: &Self::RequestContext,
    ) -> Result<FramingEncodedFinal<F>, Error>;
}

#[async_trait]
impl<F, T> ThriftService<F> for Box<T>
where
    T: ThriftService<F> + Send + Sync + ?Sized,
    F: Framing + Send + 'static,
    T::RequestContext: Send + Sync + 'static,
{
    type Handler = T::Handler;
    type RequestContext = T::RequestContext;

    async fn call(
        &self,
        req: FramingDecoded<F>,
        req_ctxt: &Self::RequestContext,
    ) -> Result<FramingEncodedFinal<F>, Error> {
        (**self).call(req, &req_ctxt).await
    }
}

/// Trait implemented by a generated type to implement a service.
#[async_trait]
pub trait ServiceProcessor<P>
where
    P: Protocol,
{
    type RequestContext;

    /// Given a method name, return a reference to the processor for that index.
    fn method_idx(&self, name: &[u8]) -> Result<usize, ApplicationException>;

    /// Given a method index and the remains of the message input, get a future
    /// for the result of the method. This will only be called if the corresponding
    /// `method_idx()` returns an (index, ServiceProcessor) tuple.
    /// `frame` is a reference to the frame containing the request.
    /// `request` is a deserializer instance set up to decode the request.
    async fn handle_method(
        &self,
        idx: usize,
        //frame: &P::Frame,
        request: &mut P::Deserializer,
        req_ctxt: &Self::RequestContext,
        seqid: u32,
    ) -> Result<ProtocolEncodedFinal<P>, Error>;
}

/// Null processor which implements no methods - it acts as the super for any service
/// which has no super-service.
#[derive(Debug, Clone)]
pub struct NullServiceProcessor<P, R> {
    _phantom: PhantomData<(P, R)>,
}

impl<P, R> NullServiceProcessor<P, R> {
    pub fn new() -> Self {
        Self {
            _phantom: PhantomData,
        }
    }
}

impl<P, R> Default for NullServiceProcessor<P, R> {
    fn default() -> Self {
        Self::new()
    }
}

#[async_trait]
impl<P, R> ServiceProcessor<P> for NullServiceProcessor<P, R>
where
    P: Protocol + Sync,
    P::Deserializer: Send,
    R: Sync,
{
    type RequestContext = R;

    #[inline]
    fn method_idx(&self, name: &[u8]) -> Result<usize, ApplicationException> {
        Err(ApplicationException::new(
            ApplicationExceptionErrorCode::UnknownMethod,
            format!("Unknown method {}", String::from_utf8_lossy(name)),
        ))
    }

    async fn handle_method(
        &self,
        _idx: usize,
        //_frame: &P::Frame,
        _d: &mut P::Deserializer,
        _req_ctxt: &R,
        _seqid: u32,
    ) -> Result<ProtocolEncodedFinal<P>, Error> {
        // Should never be called since method_idx() always returns an error
        unimplemented!("NullServiceProcessor implements no methods")
    }
}

#[async_trait]
impl<P, R> ThriftService<P::Frame> for NullServiceProcessor<P, R>
where
    P: Protocol + Send + Sync + 'static,
    P::Frame: Send + 'static,
    R: Send + Sync + 'static,
{
    type Handler = ();
    type RequestContext = R;

    async fn call(
        &self,
        req: ProtocolDecoded<P>,
        _ctxt: &R,
    ) -> Result<ProtocolEncodedFinal<P>, Error> {
        let mut p = P::deserializer(req);

        let ((name, ae), _, seqid) = p.read_message_begin(|name| {
            let name = String::from_utf8_lossy(name).into_owned();
            let ae = ApplicationException::unimplemented_method("NullService", &name);
            (name, ae)
        })?;

        let res = serialize!(P, |p| protocol::write_message(
            p,
            &name,
            MessageType::Exception,
            seqid,
            |p| ae.write(p),
        ));

        Ok(res)
    }
}
