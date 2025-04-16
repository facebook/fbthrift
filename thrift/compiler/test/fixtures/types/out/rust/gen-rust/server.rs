// @generated by Thrift for thrift/compiler/test/fixtures/types/src/module.thrift
// This file is probably not the place you want to edit!

//! Server definitions for `module`.

#![recursion_limit = "100000000"]
#![allow(non_camel_case_types, non_snake_case, non_upper_case_globals, unused_crate_dependencies, unused_imports, clippy::all)]


#[doc(inline)]
pub use :: as types;

pub mod errors {
    #[doc(inline)]
    pub use ::::services::some_service;
    #[doc(inline)]
    #[allow(ambiguous_glob_reexports)]
    pub use ::::services::some_service::*;
}

pub(crate) use crate as server;
pub(crate) use ::::services;



#[::async_trait::async_trait]
pub trait SomeService: ::std::marker::Send + ::std::marker::Sync + 'static {
    async fn bounce_map(
        &self,
        _m: included__types::SomeMap,
    ) -> ::std::result::Result<included__types::SomeMap, crate::services::some_service::BounceMapExn> {
        ::std::result::Result::Err(crate::services::some_service::BounceMapExn::ApplicationException(
            ::fbthrift::ApplicationException::unimplemented_method(
                "SomeService",
                "bounce_map",
            ),
        ))
    }
    async fn binary_keyed_map(
        &self,
        _r: ::std::vec::Vec<::std::primitive::i64>,
    ) -> ::std::result::Result<::std::collections::BTreeMap<crate::types::TBinary, ::std::primitive::i64>, crate::services::some_service::BinaryKeyedMapExn> {
        ::std::result::Result::Err(crate::services::some_service::BinaryKeyedMapExn::ApplicationException(
            ::fbthrift::ApplicationException::unimplemented_method(
                "SomeService",
                "binary_keyed_map",
            ),
        ))
    }
}

#[::async_trait::async_trait]
impl<T> SomeService for ::std::boxed::Box<T>
where
    T: SomeService + Send + Sync + ?Sized,
{
    async fn bounce_map(
        &self,
        m: included__types::SomeMap,
    ) -> ::std::result::Result<included__types::SomeMap, crate::services::some_service::BounceMapExn> {
        (**self).bounce_map(
            m,
        ).await
    }
    async fn binary_keyed_map(
        &self,
        r: ::std::vec::Vec<::std::primitive::i64>,
    ) -> ::std::result::Result<::std::collections::BTreeMap<crate::types::TBinary, ::std::primitive::i64>, crate::services::some_service::BinaryKeyedMapExn> {
        (**self).binary_keyed_map(
            r,
        ).await
    }
}

#[::async_trait::async_trait]
impl<T> SomeService for ::std::sync::Arc<T>
where
    T: SomeService + Send + Sync + ?Sized,
{
    async fn bounce_map(
        &self,
        m: included__types::SomeMap,
    ) -> ::std::result::Result<included__types::SomeMap, crate::services::some_service::BounceMapExn> {
        (**self).bounce_map(
            m,
        ).await
    }
    async fn binary_keyed_map(
        &self,
        r: ::std::vec::Vec<::std::primitive::i64>,
    ) -> ::std::result::Result<::std::collections::BTreeMap<crate::types::TBinary, ::std::primitive::i64>, crate::services::some_service::BinaryKeyedMapExn> {
        (**self).binary_keyed_map(
            r,
        ).await
    }
}
/// Processor for SomeService's methods.
#[derive(Clone, Debug)]
pub struct SomeServiceProcessor<P, H, R, RS> {
    service: H,
    supa: ::fbthrift::NullServiceProcessor<P, R, RS>,
    _phantom: ::std::marker::PhantomData<(P, H, R, RS)>,
}


struct Args_SomeService_bounce_map {
    m: included__types::SomeMap,
}

impl<P: ::fbthrift::ProtocolReader> ::fbthrift::Deserialize<P> for self::Args_SomeService_bounce_map {
    #[inline]
    #[::tracing::instrument(skip_all, level = "trace", name = "deserialize_args", fields(method = "SomeService.bounce_map"))]
    fn rs_thrift_read(p: &mut P) -> ::anyhow::Result<Self> {
        static ARGS: &[::fbthrift::Field] = &[
            ::fbthrift::Field::new("m", ::fbthrift::TType::Map, 1),
        ];
        let mut field_m = ::std::option::Option::None;
        let _ = p.read_struct_begin(|_| ())?;
        loop {
            let (_, fty, fid) = p.read_field_begin(|_| (), ARGS)?;
            match (fty, fid as ::std::primitive::i32) {
                (::fbthrift::TType::Stop, _) => break,
                (::fbthrift::TType::Map, 1) => field_m = ::std::option::Option::Some(::anyhow::Context::context(::fbthrift::Deserialize::rs_thrift_read(p), ::fbthrift::errors::DeserializingArgError { arg: "m", function: "bounce_map"})?),
                (fty, _) => p.skip(fty)?,
            }
            p.read_field_end()?;
        }
        p.read_struct_end()?;
        ::std::result::Result::Ok(Self {
            m: field_m.ok_or_else(|| ::anyhow::anyhow!("`{}` missing arg `{}`", "SomeService.bounce_map", "m"))?,
        })
    }
}


struct Args_SomeService_binary_keyed_map {
    r: ::std::vec::Vec<::std::primitive::i64>,
}

impl<P: ::fbthrift::ProtocolReader> ::fbthrift::Deserialize<P> for self::Args_SomeService_binary_keyed_map {
    #[inline]
    #[::tracing::instrument(skip_all, level = "trace", name = "deserialize_args", fields(method = "SomeService.binary_keyed_map"))]
    fn rs_thrift_read(p: &mut P) -> ::anyhow::Result<Self> {
        static ARGS: &[::fbthrift::Field] = &[
            ::fbthrift::Field::new("r", ::fbthrift::TType::List, 1),
        ];
        let mut field_r = ::std::option::Option::None;
        let _ = p.read_struct_begin(|_| ())?;
        loop {
            let (_, fty, fid) = p.read_field_begin(|_| (), ARGS)?;
            match (fty, fid as ::std::primitive::i32) {
                (::fbthrift::TType::Stop, _) => break,
                (::fbthrift::TType::List, 1) => field_r = ::std::option::Option::Some(::anyhow::Context::context(::fbthrift::Deserialize::rs_thrift_read(p), ::fbthrift::errors::DeserializingArgError { arg: "r", function: "binary_keyed_map"})?),
                (fty, _) => p.skip(fty)?,
            }
            p.read_field_end()?;
        }
        p.read_struct_end()?;
        ::std::result::Result::Ok(Self {
            r: field_r.ok_or_else(|| ::anyhow::anyhow!("`{}` missing arg `{}`", "SomeService.binary_keyed_map", "r"))?,
        })
    }
}

impl<P, H, R, RS> SomeServiceProcessor<P, H, R, RS>
where
    P: ::fbthrift::Protocol + ::std::marker::Send + ::std::marker::Sync + 'static,
    P::Frame: ::std::marker::Send + 'static,
    P::Deserializer: ::std::marker::Send,
    H: SomeService,
    R: ::fbthrift::RequestContext<Name = ::std::ffi::CStr> + ::std::marker::Send + ::std::marker::Sync + 'static,
    RS: ::fbthrift::ReplyState<P::Frame, RequestContext = R> + ::std::marker::Send + ::std::marker::Sync + 'static,
    <R as ::fbthrift::RequestContext>::ContextStack: ::fbthrift::ContextStack<Name = R::Name, Frame = <P as ::fbthrift::Protocol>::Frame>
        + ::std::marker::Send + ::std::marker::Sync,
    ::fbthrift::ProtocolDecoded<P>: ::std::clone::Clone,
    ::fbthrift::ProtocolEncodedFinal<P>: ::std::clone::Clone + ::fbthrift::BufExt,
{
    pub fn new(service: H) -> Self {
        Self {
            service,
            supa: ::fbthrift::NullServiceProcessor::new(),
            _phantom: ::std::marker::PhantomData,
        }
    }

    pub fn into_inner(self) -> H {
        self.service
    }

    #[::tracing::instrument(skip_all, name = "handler", fields(method = "SomeService.bounce_map"))]
    async fn handle_bounce_map<'a>(
        &'a self,
        p: &'a mut P::Deserializer,
        req: ::fbthrift::ProtocolDecoded<P>,
        req_ctxt: &R,
        reply_state: ::std::sync::Arc<RS>,
        _seqid: ::std::primitive::u32,
    ) -> ::anyhow::Result<()> {
        use ::futures::FutureExt as _;

        const SERVICE_NAME: &::std::ffi::CStr = c"SomeService";
        const METHOD_NAME: &::std::ffi::CStr = c"bounce_map";
        const SERVICE_METHOD_NAME: &::std::ffi::CStr = c"SomeService.bounce_map";
        let mut ctx_stack = req_ctxt.get_context_stack(SERVICE_NAME, SERVICE_METHOD_NAME)?;
        ::fbthrift::ContextStack::pre_read(&mut ctx_stack)?;
        let _args: self::Args_SomeService_bounce_map = ::fbthrift::Deserialize::rs_thrift_read(p)?;
        let bytes_read = ::fbthrift::help::buf_len(&req)?;
        ::fbthrift::ContextStack::on_read_data(&mut ctx_stack, ::fbthrift::SerializedMessage {
            protocol: P::PROTOCOL_ID,
            method_name: METHOD_NAME,
            buffer: req,
        })?;
        ::fbthrift::ContextStack::post_read(&mut ctx_stack, bytes_read)?;

        let res = ::std::panic::AssertUnwindSafe(
            self.service.bounce_map(
                _args.m,
            )
        )
        .catch_unwind()
        .await;

        // nested results - panic catch on the outside, method on the inside
        let res = match res {
            ::std::result::Result::Ok(::std::result::Result::Ok(res)) => {
                ::tracing::trace!(method = "SomeService.bounce_map", "success");
                ::std::result::Result::Ok(res)
            }
            ::std::result::Result::Ok(::std::result::Result::Err(exn)) => {
                ::tracing::error!(method = "SomeService.bounce_map", exception = ?exn);
                ::std::result::Result::Err(exn)
            }
            ::std::result::Result::Err(exn) => {
                let aexn = ::fbthrift::ApplicationException::handler_panic("SomeService.bounce_map", exn);
                ::tracing::error!(method = "SomeService.bounce_map", panic = ?aexn);
                ::std::result::Result::Err(crate::services::some_service::BounceMapExn::ApplicationException(aexn))
            }
        };

        let env = ::fbthrift::help::serialize_result_envelope::<P, R, crate::services::some_service::BounceMapExn>(
            "bounce_map",
            METHOD_NAME,
            _seqid,
            req_ctxt,
            &mut ctx_stack,
            res,
        )?;
        reply_state.send_reply(env);
        ::std::result::Result::Ok(())
    }

    #[::tracing::instrument(skip_all, name = "handler", fields(method = "SomeService.binary_keyed_map"))]
    async fn handle_binary_keyed_map<'a>(
        &'a self,
        p: &'a mut P::Deserializer,
        req: ::fbthrift::ProtocolDecoded<P>,
        req_ctxt: &R,
        reply_state: ::std::sync::Arc<RS>,
        _seqid: ::std::primitive::u32,
    ) -> ::anyhow::Result<()> {
        use ::futures::FutureExt as _;

        const SERVICE_NAME: &::std::ffi::CStr = c"SomeService";
        const METHOD_NAME: &::std::ffi::CStr = c"binary_keyed_map";
        const SERVICE_METHOD_NAME: &::std::ffi::CStr = c"SomeService.binary_keyed_map";
        let mut ctx_stack = req_ctxt.get_context_stack(SERVICE_NAME, SERVICE_METHOD_NAME)?;
        ::fbthrift::ContextStack::pre_read(&mut ctx_stack)?;
        let _args: self::Args_SomeService_binary_keyed_map = ::fbthrift::Deserialize::rs_thrift_read(p)?;
        let bytes_read = ::fbthrift::help::buf_len(&req)?;
        ::fbthrift::ContextStack::on_read_data(&mut ctx_stack, ::fbthrift::SerializedMessage {
            protocol: P::PROTOCOL_ID,
            method_name: METHOD_NAME,
            buffer: req,
        })?;
        ::fbthrift::ContextStack::post_read(&mut ctx_stack, bytes_read)?;

        let res = ::std::panic::AssertUnwindSafe(
            self.service.binary_keyed_map(
                _args.r,
            )
        )
        .catch_unwind()
        .await;

        // nested results - panic catch on the outside, method on the inside
        let res = match res {
            ::std::result::Result::Ok(::std::result::Result::Ok(res)) => {
                ::tracing::trace!(method = "SomeService.binary_keyed_map", "success");
                ::std::result::Result::Ok(res)
            }
            ::std::result::Result::Ok(::std::result::Result::Err(exn)) => {
                ::tracing::error!(method = "SomeService.binary_keyed_map", exception = ?exn);
                ::std::result::Result::Err(exn)
            }
            ::std::result::Result::Err(exn) => {
                let aexn = ::fbthrift::ApplicationException::handler_panic("SomeService.binary_keyed_map", exn);
                ::tracing::error!(method = "SomeService.binary_keyed_map", panic = ?aexn);
                ::std::result::Result::Err(crate::services::some_service::BinaryKeyedMapExn::ApplicationException(aexn))
            }
        };

        let env = ::fbthrift::help::serialize_result_envelope::<P, R, crate::services::some_service::BinaryKeyedMapExn>(
            "binary_keyed_map",
            METHOD_NAME,
            _seqid,
            req_ctxt,
            &mut ctx_stack,
            res,
        )?;
        reply_state.send_reply(env);
        ::std::result::Result::Ok(())
    }
}

#[::async_trait::async_trait]
impl<P, H, R, RS> ::fbthrift::ServiceProcessor<P> for SomeServiceProcessor<P, H, R, RS>
where
    P: ::fbthrift::Protocol + ::std::marker::Send + ::std::marker::Sync + 'static,
    P::Deserializer: ::std::marker::Send,
    H: SomeService,
    P::Frame: ::std::marker::Send + 'static,
    R: ::fbthrift::RequestContext<Name = ::std::ffi::CStr> + ::std::marker::Send + ::std::marker::Sync + 'static,
    <R as ::fbthrift::RequestContext>::ContextStack: ::fbthrift::ContextStack<Name = R::Name, Frame = <P as ::fbthrift::Protocol>::Frame>
        + ::std::marker::Send + ::std::marker::Sync + 'static,
    RS: ::fbthrift::ReplyState<P::Frame, RequestContext = R> + ::std::marker::Send + ::std::marker::Sync + 'static,
    ::fbthrift::ProtocolDecoded<P>: ::std::clone::Clone,
    ::fbthrift::ProtocolEncodedFinal<P>: ::std::clone::Clone + ::fbthrift::BufExt,
{
    type RequestContext = R;
    type ReplyState = RS;

    #[inline]
    fn method_idx(&self, name: &[::std::primitive::u8]) -> ::std::result::Result<::std::primitive::usize, ::fbthrift::ApplicationException> {
        match name {
            b"bounce_map" => ::std::result::Result::Ok(0usize),
            b"binary_keyed_map" => ::std::result::Result::Ok(1usize),
            _ => ::std::result::Result::Err(::fbthrift::ApplicationException::unknown_method()),
        }
    }

    #[allow(clippy::match_single_binding)]
    async fn handle_method(
        &self,
        idx: ::std::primitive::usize,
        _p: &mut P::Deserializer,
        _req: ::fbthrift::ProtocolDecoded<P>,
        _req_ctxt: &R,
        _reply_state: ::std::sync::Arc<RS>,
        _seqid: ::std::primitive::u32,
    ) -> ::anyhow::Result<()> {
        match idx {
            0usize => {
                self.handle_bounce_map(_p, _req, _req_ctxt, _reply_state, _seqid).await
            }
            1usize => {
                self.handle_binary_keyed_map(_p, _req, _req_ctxt, _reply_state, _seqid).await
            }
            bad => panic!(
                "{}: unexpected method idx {}",
                "SomeServiceProcessor",
                bad
            ),
        }
    }

    #[allow(clippy::match_single_binding)]
    #[inline]
    fn create_interaction_idx(&self, name: &::std::primitive::str) -> ::anyhow::Result<::std::primitive::usize> {
        match name {
            _ => ::anyhow::bail!("Unknown interaction"),
        }
    }

    #[allow(clippy::match_single_binding)]
    fn handle_create_interaction(
        &self,
        idx: ::std::primitive::usize,
    ) -> ::anyhow::Result<
        ::std::sync::Arc<dyn ::fbthrift::ThriftService<P::Frame, Handler = (), RequestContext = Self::RequestContext, ReplyState = Self::ReplyState> + ::std::marker::Send + 'static>
    > {
        match idx {
            bad => panic!(
                "{}: unexpected method idx {}",
                "SomeServiceProcessor",
                bad
            ),
        }
    }

    async fn handle_on_termination(&self) {
    }
}

#[::async_trait::async_trait]
impl<P, H, R, RS> ::fbthrift::ThriftService<P::Frame> for SomeServiceProcessor<P, H, R, RS>
where
    P: ::fbthrift::Protocol + ::std::marker::Send + ::std::marker::Sync + 'static,
    P::Deserializer: ::std::marker::Send,
    P::Frame: ::std::marker::Send + 'static,
    H: SomeService,
    R: ::fbthrift::RequestContext<Name = ::std::ffi::CStr> + ::std::marker::Send + ::std::marker::Sync + 'static,
    <R as ::fbthrift::RequestContext>::ContextStack: ::fbthrift::ContextStack<Name = R::Name, Frame = <P as ::fbthrift::Protocol>::Frame>
        + ::std::marker::Send + ::std::marker::Sync + 'static,
    RS: ::fbthrift::ReplyState<P::Frame, RequestContext = R> + ::std::marker::Send + ::std::marker::Sync + 'static,
    ::fbthrift::ProtocolDecoded<P>: ::std::clone::Clone,
    ::fbthrift::ProtocolEncodedFinal<P>: ::std::clone::Clone + ::fbthrift::BufExt,
{
    type Handler = H;
    type RequestContext = R;
    type ReplyState = RS;

    #[tracing::instrument(level="trace", skip_all, fields(service = "SomeService"))]
    async fn call(
        &self,
        req: ::fbthrift::ProtocolDecoded<P>,
        req_ctxt: &R,
        reply_state: ::std::sync::Arc<RS>,
    ) -> ::anyhow::Result<()> {
        use ::fbthrift::{ProtocolReader as _, ServiceProcessor as _};
        let mut p = P::deserializer(req.clone());
        let (idx, mty, seqid) = p.read_message_begin(|name| self.method_idx(name))?;
        if mty != ::fbthrift::MessageType::Call {
            return ::std::result::Result::Err(::std::convert::From::from(::fbthrift::ApplicationException::new(
                ::fbthrift::ApplicationExceptionErrorCode::InvalidMessageType,
                format!("message type {:?} not handled", mty)
            )));
        }
        let idx = match idx {
            ::std::result::Result::Ok(idx) => idx,
            ::std::result::Result::Err(_) => {
                return self.supa.call(req, req_ctxt, reply_state).await;
            }
        };
        self.handle_method(idx, &mut p, req, req_ctxt, reply_state, seqid).await?;
        p.read_message_end()?;

        ::std::result::Result::Ok(())
    }

    fn create_interaction(
        &self,
        name: &::std::primitive::str,
    ) -> ::anyhow::Result<
        ::std::sync::Arc<dyn ::fbthrift::ThriftService<P::Frame, Handler = (), RequestContext = R, ReplyState = RS> + ::std::marker::Send + 'static>
    > {
        use ::fbthrift::{ServiceProcessor as _};
        let idx = self.create_interaction_idx(name);
        let idx = match idx {
            ::anyhow::Result::Ok(idx) => idx,
            ::anyhow::Result::Err(_) => {
                return self.supa.create_interaction(name);
            }
        };
        self.handle_create_interaction(idx)
    }

    fn get_method_metadata(&self) -> &'static [::fbthrift::processor::MethodMetadata] {
        &[
            // From module.SomeService:
            ::fbthrift::processor::MethodMetadata{
              interaction_type: ::fbthrift::processor::InteractionType::None,
              rpc_kind: ::fbthrift::processor::RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE,
              name: "bounce_map",
              starts_interaction: false,
              interaction_name: None,
            },
            ::fbthrift::processor::MethodMetadata{
              interaction_type: ::fbthrift::processor::InteractionType::None,
              rpc_kind: ::fbthrift::processor::RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE,
              name: "binary_keyed_map",
              starts_interaction: false,
              interaction_name: None,
            },
        ]
    }

    async fn on_termination(&self) {
        use ::fbthrift::{ServiceProcessor as _};
        self.handle_on_termination().await
    }
}

/// Construct a new instance of a SomeService service.
///
/// This is called when a new instance of a Thrift service Processor
/// is needed for a particular Thrift protocol.
#[::tracing::instrument(level="debug", skip_all, fields(proto = ?proto))]
pub fn make_SomeService_server<F, H, R, RS>(
    proto: ::fbthrift::ProtocolID,
    handler: H,
) -> ::std::result::Result<::std::boxed::Box<dyn ::fbthrift::ThriftService<F, Handler = H, RequestContext = R, ReplyState = RS> + ::std::marker::Send + 'static>, ::fbthrift::ApplicationException>
where
    F: ::fbthrift::Framing + ::std::marker::Send + ::std::marker::Sync + 'static,
    H: SomeService,
    R: ::fbthrift::RequestContext<Name = ::std::ffi::CStr> + ::std::marker::Send + ::std::marker::Sync + 'static,
    <R as ::fbthrift::RequestContext>::ContextStack: ::fbthrift::ContextStack<Name = R::Name, Frame = F> + ::std::marker::Send + ::std::marker::Sync + 'static,
    RS: ::fbthrift::ReplyState<F, RequestContext = R> + ::std::marker::Send + ::std::marker::Sync + 'static,
    ::fbthrift::FramingDecoded<F>: ::std::clone::Clone,
    ::fbthrift::FramingEncodedFinal<F>: ::std::clone::Clone + ::fbthrift::BufExt,
{
    match proto {
        ::fbthrift::ProtocolID::BinaryProtocol => {
            ::std::result::Result::Ok(::std::boxed::Box::new(SomeServiceProcessor::<::fbthrift::BinaryProtocol<F>, H, R, RS>::new(handler)))
        }
        ::fbthrift::ProtocolID::CompactProtocol => {
            ::std::result::Result::Ok(::std::boxed::Box::new(SomeServiceProcessor::<::fbthrift::CompactProtocol<F>, H, R, RS>::new(handler)))
        }
        bad => {
            ::tracing::error!(method = "SomeService.", invalid_protocol = ?bad);
            ::std::result::Result::Err(::fbthrift::ApplicationException::invalid_protocol(bad))
        }
    }
}

