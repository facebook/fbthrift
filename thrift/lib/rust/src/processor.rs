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

use crate::application_exception::{ApplicationException, ApplicationExceptionErrorCode};
use crate::context_stack::ContextStack;
use crate::exceptions::{ExceptionInfo, ResultInfo};
use crate::framing::{Framing, FramingDecoded, FramingEncodedFinal};
use crate::protocol::{
    Protocol, ProtocolDecoded, ProtocolEncodedFinal, ProtocolReader, ProtocolWriter,
};
use crate::request_context::RequestContext;
use crate::serialize::Serialize;
use crate::ttype::TType;
use anyhow::{bail, Error};
use async_trait::async_trait;
use std::ffi::CStr;
use std::marker::PhantomData;
use std::sync::Arc;

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

    fn create_interaction(
        &self,
        _name: &str,
    ) -> ::anyhow::Result<
        Arc<
            dyn ThriftService<F, Handler = (), RequestContext = Self::RequestContext>
                + ::std::marker::Send
                + 'static,
        >,
    > {
        bail!("Thrift server does not support interactions");
    }
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

    fn create_interaction(
        &self,
        name: &str,
    ) -> ::anyhow::Result<
        Arc<
            dyn ThriftService<F, Handler = (), RequestContext = Self::RequestContext>
                + ::std::marker::Send
                + 'static,
        >,
    > {
        (**self).create_interaction(name)
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


    /// Given a method name, return a reference to the interaction creation fn for that index
    fn create_interaction_idx(&self, _name: &str) -> ::anyhow::Result<::std::primitive::usize> {
        bail!("Processor does not support interactions");
    }

    /// Given a creation method index, it produces a fresh interaction processor
    fn handle_create_interaction(
        &self,
        _idx: ::std::primitive::usize,
    ) -> ::anyhow::Result<
        Arc<
            dyn ThriftService<P::Frame, Handler = (), RequestContext = Self::RequestContext>
                + ::std::marker::Send
                + 'static,
        >,
    > {
        bail!("Processor does not support interactions");
    }
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

    fn create_interaction_idx(&self, name: &str) -> ::anyhow::Result<::std::primitive::usize> {
        bail!("Unknown interaction {}", name);
    }

    fn handle_create_interaction(
        &self,
        _idx: ::std::primitive::usize,
    ) -> ::anyhow::Result<
        Arc<
            dyn ThriftService<P::Frame, Handler = (), RequestContext = Self::RequestContext>
                + ::std::marker::Send
                + 'static,
        >,
    > {
        unimplemented!("NullServiceProcessor implements no interactions")
    }
}

#[async_trait]
impl<P, R> ThriftService<P::Frame> for NullServiceProcessor<P, R>
where
    P: Protocol + Send + Sync + 'static,
    P::Frame: Send + 'static,
    R: RequestContext<Name = CStr> + Send + Sync + 'static,
    R::ContextStack: ContextStack<Name = CStr>,
{
    type Handler = ();
    type RequestContext = R;

    async fn call(
        &self,
        req: ProtocolDecoded<P>,
        rctxt: &R,
    ) -> Result<ProtocolEncodedFinal<P>, Error> {
        let mut p = P::deserializer(req);

        const SERVICE_NAME: &str = "NullService";
        let ((name, ae), _, seqid) = p.read_message_begin(|name| {
            let name = String::from_utf8_lossy(name).into_owned();
            let ae = ApplicationException::unimplemented_method(SERVICE_NAME, &name);
            (name, ae)
        })?;

        p.skip(TType::Struct)?;
        p.read_message_end()?;

        rctxt.set_user_exception_header(ae.exn_name(), &ae.exn_value())?;
        let res = serialize!(P, |p| {
            p.write_message_begin(&name, ae.result_type().message_type(), seqid);
            ae.write(p);
            p.write_message_end();
        });
        Ok(res)
    }

    fn create_interaction(
        &self,
        name: &str,
    ) -> ::anyhow::Result<
        Arc<
            dyn ThriftService<
                    P::Frame,
                    Handler = Self::Handler,
                    RequestContext = Self::RequestContext,
                > + ::std::marker::Send
                + 'static,
        >,
    > {
        bail!("Unimplemented interaction {}", name);
    }
}
