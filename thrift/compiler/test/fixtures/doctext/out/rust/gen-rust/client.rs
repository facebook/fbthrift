// @generated by Thrift for thrift/compiler/test/fixtures/doctext/src/module.thrift
// This file is probably not the place you want to edit!

//! Client implementation for each service in `module`.

#![recursion_limit = "100000000"]
#![allow(non_camel_case_types, non_snake_case, non_upper_case_globals, unused_crate_dependencies, unused_imports, clippy::all)]


#[doc(inline)]
pub use :: as types;

pub mod errors;

pub(crate) use crate as client;
pub(crate) use ::::services;


#[doc = "Detailed overview of service"]
pub trait C: ::std::marker::Send {
    #[doc = "Function doctext."]
    fn f(
        &self,
    ) -> ::futures::future::BoxFuture<'static, ::std::result::Result<(), crate::errors::c::FError>>;

    #[doc = "Streaming function"]
    fn numbers(
        &self,
    ) -> ::futures::future::BoxFuture<'static, ::std::result::Result<::futures::stream::BoxStream<'static, ::std::result::Result<crate::types::number, crate::errors::c::NumbersStreamError>>, crate::errors::c::NumbersError>>;

    #[doc = ""]
    fn thing(
        &self,
        arg_a: ::std::primitive::i32,
        arg_b: &::std::primitive::str,
        arg_c: &::std::collections::BTreeSet<::std::primitive::i32>,
    ) -> ::futures::future::BoxFuture<'static, ::std::result::Result<::std::string::String, crate::errors::c::ThingError>>;
}

pub trait CExt<T>: C
where
    T: ::fbthrift::Transport,
{
    #[doc = "Function doctext."]
    fn f_with_rpc_opts(
        &self,
        rpc_options: T::RpcOptions,
    ) -> ::futures::future::BoxFuture<'static, ::std::result::Result<(), crate::errors::c::FError>>;
    #[doc = "Streaming function"]
    fn numbers_with_rpc_opts(
        &self,
        rpc_options: T::RpcOptions,
    ) -> ::futures::future::BoxFuture<'static, ::std::result::Result<::futures::stream::BoxStream<'static, ::std::result::Result<crate::types::number, crate::errors::c::NumbersStreamError>>, crate::errors::c::NumbersError>>;
    #[doc = ""]
    fn thing_with_rpc_opts(
        &self,
        arg_a: ::std::primitive::i32,
        arg_b: &::std::primitive::str,
        arg_c: &::std::collections::BTreeSet<::std::primitive::i32>,
        rpc_options: T::RpcOptions,
    ) -> ::futures::future::BoxFuture<'static, ::std::result::Result<::std::string::String, crate::errors::c::ThingError>>;

    fn transport(&self) -> &T;
}

#[allow(deprecated)]
impl<'a, S> C for S
where
    S: ::std::convert::AsRef<dyn C + 'a>,
    S: ::std::marker::Send,
{
    fn f(
        &self,
    ) -> ::futures::future::BoxFuture<'static, ::std::result::Result<(), crate::errors::c::FError>> {
        self.as_ref().f(
        )
    }
    fn numbers(
        &self,
    ) -> ::futures::future::BoxFuture<'static, ::std::result::Result<::futures::stream::BoxStream<'static, ::std::result::Result<crate::types::number, crate::errors::c::NumbersStreamError>>, crate::errors::c::NumbersError>> {
        self.as_ref().numbers(
        )
    }
    fn thing(
        &self,
        arg_a: ::std::primitive::i32,
        arg_b: &::std::primitive::str,
        arg_c: &::std::collections::BTreeSet<::std::primitive::i32>,
    ) -> ::futures::future::BoxFuture<'static, ::std::result::Result<::std::string::String, crate::errors::c::ThingError>> {
        self.as_ref().thing(
            arg_a,
            arg_b,
            arg_c,
        )
    }
}

#[allow(deprecated)]
impl<'a, S, T> CExt<T> for S
where
    S: ::std::convert::AsRef<dyn C + 'a> + ::std::convert::AsRef<dyn CExt<T> + 'a>,
    S: ::std::marker::Send + ::fbthrift::help::GetTransport<T>,
    T: ::fbthrift::Transport,
{
    fn f_with_rpc_opts(
        &self,
        rpc_options: T::RpcOptions,
    ) -> ::futures::future::BoxFuture<'static, ::std::result::Result<(), crate::errors::c::FError>> {
        <Self as ::std::convert::AsRef<dyn CExt<T>>>::as_ref(self).f_with_rpc_opts(
            rpc_options,
        )
    }
    fn numbers_with_rpc_opts(
        &self,
        rpc_options: T::RpcOptions,
    ) -> ::futures::future::BoxFuture<'static, ::std::result::Result<::futures::stream::BoxStream<'static, ::std::result::Result<crate::types::number, crate::errors::c::NumbersStreamError>>, crate::errors::c::NumbersError>> {
        <Self as ::std::convert::AsRef<dyn CExt<T>>>::as_ref(self).numbers_with_rpc_opts(
            rpc_options,
        )
    }
    fn thing_with_rpc_opts(
        &self,
        arg_a: ::std::primitive::i32,
        arg_b: &::std::primitive::str,
        arg_c: &::std::collections::BTreeSet<::std::primitive::i32>,
        rpc_options: T::RpcOptions,
    ) -> ::futures::future::BoxFuture<'static, ::std::result::Result<::std::string::String, crate::errors::c::ThingError>> {
        <Self as ::std::convert::AsRef<dyn CExt<T>>>::as_ref(self).thing_with_rpc_opts(
            arg_a,
            arg_b,
            arg_c,
            rpc_options,
        )
    }

    fn transport(&self) -> &T {
        ::fbthrift::help::GetTransport::transport(self)
    }
}
/// Client definitions for `C`.
pub struct CImpl<P, T, S = ::fbthrift::NoopSpawner> {
    transport: T,
    _phantom: ::std::marker::PhantomData<fn() -> (P, S)>,
}


impl<P, T, S> CImpl<P, T, S>
where
    P: ::fbthrift::Protocol,
    T: ::fbthrift::Transport,
    P::Frame: ::fbthrift::Framing<DecBuf = ::fbthrift::FramingDecoded<T>>,
    ::fbthrift::ProtocolEncoded<P>: ::fbthrift::BufMutExt<Final = ::fbthrift::FramingEncodedFinal<T>>,
    P::Deserializer: ::std::marker::Send,
    S: ::fbthrift::help::Spawner,
{
    pub fn new(
        transport: T,
    ) -> Self {
        Self {
            transport,
            _phantom: ::std::marker::PhantomData,
        }
    }

    pub fn transport(&self) -> &T {
        ::fbthrift::help::GetTransport::transport(self)
    }



    fn _f_impl(
        &self,
        rpc_options: T::RpcOptions,
    ) -> ::futures::future::BoxFuture<'static, ::std::result::Result<(), crate::errors::c::FError>> {
        use ::tracing::Instrument as _;
        use ::futures::FutureExt as _;

        let service_name = c"C";
        let service_method_name = c"C.f";

        let args = self::Args_C_f {
            _phantom: ::std::marker::PhantomData,
        };

        let transport = self.transport();

        // need to do call setup outside of async block because T: Transport isn't Send
        let request_env = match ::fbthrift::help::serialize_request_envelope::<P, _>("f", &args) {
            ::std::result::Result::Ok(res) => res,
            ::std::result::Result::Err(err) => return ::futures::future::err(err.into()).boxed(),
        };

        let call = transport
            .call(service_name, service_method_name, request_env, rpc_options)
            .instrument(::tracing::trace_span!("call", method = "C.f"));

        async move {
            let reply_env = call.await?;

            let de = P::deserializer(reply_env);
            let res = ::fbthrift::help::async_deserialize_response_envelope::<P, crate::errors::c::FReader, S>(de).await?;

            let res = match res {
                ::std::result::Result::Ok(res) => res,
                ::std::result::Result::Err(aexn) => {
                    ::std::result::Result::Err(crate::errors::c::FError::ApplicationException(aexn))
                }
            };
            res
        }
        .instrument(::tracing::info_span!("stream", method = "C.f"))
        .boxed()
    }

    fn _numbers_impl(
        &self,
        rpc_options: T::RpcOptions,
    ) -> ::futures::future::BoxFuture<'static, ::std::result::Result<::futures::stream::BoxStream<'static, ::std::result::Result<crate::types::number, crate::errors::c::NumbersStreamError>>, crate::errors::c::NumbersError>> {
        use ::tracing::Instrument as _;
        use ::futures::FutureExt as _;
        use ::futures::StreamExt as _;
        use ::fbthrift::Deserialize as _;

        let service_name = c"C";
        let service_method_name = c"C.numbers";

        let args = self::Args_C_numbers {
            _phantom: ::std::marker::PhantomData,
        };

        let transport = self.transport();

        // need to do call setup outside of async block because T: Transport isn't Send
        let request_env = match ::fbthrift::help::serialize_request_envelope::<P, _>("numbers", &args) {
            ::std::result::Result::Ok(res) => res,
            ::std::result::Result::Err(err) => return ::futures::future::err(err.into()).boxed(),
        };

        let call_stream = transport
            .call_stream(service_name, service_method_name, request_env, rpc_options)
            .instrument(::tracing::trace_span!("call_stream", method = "C.numbers"));

        async move {
            let (initial, stream) = call_stream.await?;

            let new_stream = stream.then(|item_res| {
                async move {
                    match item_res {
                        ::std::result::Result::Err(err) =>
                            ::std::result::Result::Err(crate::errors::c::NumbersStreamError::from(err)),
                        ::std::result::Result::Ok(item_enc) => {
                            S::spawn(move || {
                                match item_enc {
                                    ::fbthrift::ClientStreamElement::Reply(payload) => {
                                        let mut de = P::deserializer(payload);
                                        <crate::errors::c::NumbersStreamReader as ::fbthrift::help::DeserializeExn>::read_result(&mut de)
                                    }
                                    ::fbthrift::ClientStreamElement::ApplicationEx(payload) => {
                                        let mut de = P::deserializer(payload);
                                        let aexn = ::fbthrift::ApplicationException::rs_thrift_read(&mut de)?;
                                        ::std::result::Result::Ok(::std::result::Result::Err(crate::errors::c::NumbersStreamError::ApplicationException(aexn)))
                                    }
                                    ::fbthrift::ClientStreamElement::DeclaredEx(payload) => {
                                        let mut de = P::deserializer(payload);
                                        <crate::errors::c::NumbersStreamReader as ::fbthrift::help::DeserializeExn>::read_result(&mut de)
                                    }
                                }
                            }).await.map_err(::anyhow::Error::from)??
                        }
                    }
                }
            })
            .boxed();

            let de = P::deserializer(initial);
            let res = ::fbthrift::help::async_deserialize_response_envelope::<P, crate::errors::c::NumbersReader, S>(de)
                .await??
                .map(move |_| new_stream);
            res
        }
        .instrument(::tracing::info_span!("stream", method = "C.numbers"))
        .boxed()
    }

    fn _thing_impl(
        &self,
        arg_a: ::std::primitive::i32,
        arg_b: &::std::primitive::str,
        arg_c: &::std::collections::BTreeSet<::std::primitive::i32>,
        rpc_options: T::RpcOptions,
    ) -> ::futures::future::BoxFuture<'static, ::std::result::Result<::std::string::String, crate::errors::c::ThingError>> {
        use ::tracing::Instrument as _;
        use ::futures::FutureExt as _;

        let service_name = c"C";
        let service_method_name = c"C.thing";

        let args = self::Args_C_thing {
            a: arg_a,
            b: arg_b,
            c: arg_c,
            _phantom: ::std::marker::PhantomData,
        };

        let transport = self.transport();

        // need to do call setup outside of async block because T: Transport isn't Send
        let request_env = match ::fbthrift::help::serialize_request_envelope::<P, _>("thing", &args) {
            ::std::result::Result::Ok(res) => res,
            ::std::result::Result::Err(err) => return ::futures::future::err(err.into()).boxed(),
        };

        let call = transport
            .call(service_name, service_method_name, request_env, rpc_options)
            .instrument(::tracing::trace_span!("call", method = "C.thing"));

        async move {
            let reply_env = call.await?;

            let de = P::deserializer(reply_env);
            let res = ::fbthrift::help::async_deserialize_response_envelope::<P, crate::errors::c::ThingReader, S>(de).await?;

            let res = match res {
                ::std::result::Result::Ok(res) => res,
                ::std::result::Result::Err(aexn) => {
                    ::std::result::Result::Err(crate::errors::c::ThingError::ApplicationException(aexn))
                }
            };
            res
        }
        .instrument(::tracing::info_span!("stream", method = "C.thing"))
        .boxed()
    }
}

impl<P, T, S> ::fbthrift::help::GetTransport<T> for CImpl<P, T, S>
where
    T: ::fbthrift::Transport,
{
    fn transport(&self) -> &T {
        &self.transport
    }
}



struct Args_C_f<'a> {
    _phantom: ::std::marker::PhantomData<&'a ()>,
}

impl<'a, P: ::fbthrift::ProtocolWriter> ::fbthrift::Serialize<P> for self::Args_C_f<'a> {
    #[inline]
    #[::tracing::instrument(skip_all, level = "trace", name = "serialize_args", fields(method = "C.f"))]
    fn rs_thrift_write(&self, p: &mut P) {
        p.write_struct_begin("args");
        p.write_field_stop();
        p.write_struct_end();
    }
}

struct Args_C_numbers<'a> {
    _phantom: ::std::marker::PhantomData<&'a ()>,
}

impl<'a, P: ::fbthrift::ProtocolWriter> ::fbthrift::Serialize<P> for self::Args_C_numbers<'a> {
    #[inline]
    #[::tracing::instrument(skip_all, level = "trace", name = "serialize_args", fields(method = "C.numbers"))]
    fn rs_thrift_write(&self, p: &mut P) {
        p.write_struct_begin("args");
        p.write_field_stop();
        p.write_struct_end();
    }
}

struct Args_C_thing<'a> {
    a: ::std::primitive::i32,
    b: &'a ::std::primitive::str,
    c: &'a ::std::collections::BTreeSet<::std::primitive::i32>,
    _phantom: ::std::marker::PhantomData<&'a ()>,
}

impl<'a, P: ::fbthrift::ProtocolWriter> ::fbthrift::Serialize<P> for self::Args_C_thing<'a> {
    #[inline]
    #[::tracing::instrument(skip_all, level = "trace", name = "serialize_args", fields(method = "C.thing"))]
    fn rs_thrift_write(&self, p: &mut P) {
        p.write_struct_begin("args");
        p.write_field_begin("a", ::fbthrift::TType::I32, 1i16);
        ::fbthrift::Serialize::rs_thrift_write(&self.a, p);
        p.write_field_end();
        p.write_field_begin("b", ::fbthrift::TType::String, 2i16);
        ::fbthrift::Serialize::rs_thrift_write(&self.b, p);
        p.write_field_end();
        p.write_field_begin("c", ::fbthrift::TType::Set, 3i16);
        ::fbthrift::Serialize::rs_thrift_write(&self.c, p);
        p.write_field_end();
        p.write_field_stop();
        p.write_struct_end();
    }
}

impl<P, T, S> C for CImpl<P, T, S>
where
    P: ::fbthrift::Protocol,
    T: ::fbthrift::Transport,
    P::Frame: ::fbthrift::Framing<DecBuf = ::fbthrift::FramingDecoded<T>>,
    ::fbthrift::ProtocolEncoded<P>: ::fbthrift::BufMutExt<Final = ::fbthrift::FramingEncodedFinal<T>>,
    P::Deserializer: ::std::marker::Send,
    S: ::fbthrift::help::Spawner,
{
    fn f(
        &self,
    ) -> ::futures::future::BoxFuture<'static, ::std::result::Result<(), crate::errors::c::FError>> {
        let rpc_options = T::RpcOptions::default();
        self._f_impl(
            rpc_options,
        )
    }
    fn numbers(
        &self,
    ) -> ::futures::future::BoxFuture<'static, ::std::result::Result<::futures::stream::BoxStream<'static, ::std::result::Result<crate::types::number, crate::errors::c::NumbersStreamError>>, crate::errors::c::NumbersError>> {
        let rpc_options = T::RpcOptions::default();
        self._numbers_impl(
            rpc_options,
        )
    }
    fn thing(
        &self,
        arg_a: ::std::primitive::i32,
        arg_b: &::std::primitive::str,
        arg_c: &::std::collections::BTreeSet<::std::primitive::i32>,
    ) -> ::futures::future::BoxFuture<'static, ::std::result::Result<::std::string::String, crate::errors::c::ThingError>> {
        let rpc_options = T::RpcOptions::default();
        self._thing_impl(
            arg_a,
            arg_b,
            arg_c,
            rpc_options,
        )
    }
}

impl<P, T, S> CExt<T> for CImpl<P, T, S>
where
    P: ::fbthrift::Protocol,
    T: ::fbthrift::Transport,
    P::Frame: ::fbthrift::Framing<DecBuf = ::fbthrift::FramingDecoded<T>>,
    ::fbthrift::ProtocolEncoded<P>: ::fbthrift::BufMutExt<Final = ::fbthrift::FramingEncodedFinal<T>>,
    P::Deserializer: ::std::marker::Send,
    S: ::fbthrift::help::Spawner,
{
    fn f_with_rpc_opts(
        &self,
        rpc_options: T::RpcOptions,
    ) -> ::futures::future::BoxFuture<'static, ::std::result::Result<(), crate::errors::c::FError>> {
        self._f_impl(
            rpc_options,
        )
    }
    fn numbers_with_rpc_opts(
        &self,
        rpc_options: T::RpcOptions,
    ) -> ::futures::future::BoxFuture<'static, ::std::result::Result<::futures::stream::BoxStream<'static, ::std::result::Result<crate::types::number, crate::errors::c::NumbersStreamError>>, crate::errors::c::NumbersError>> {
        self._numbers_impl(
            rpc_options,
        )
    }
    fn thing_with_rpc_opts(
        &self,
        arg_a: ::std::primitive::i32,
        arg_b: &::std::primitive::str,
        arg_c: &::std::collections::BTreeSet<::std::primitive::i32>,
        rpc_options: T::RpcOptions,
    ) -> ::futures::future::BoxFuture<'static, ::std::result::Result<::std::string::String, crate::errors::c::ThingError>> {
        self._thing_impl(
            arg_a,
            arg_b,
            arg_c,
            rpc_options,
        )
    }

    fn transport(&self) -> &T {
        self.transport()
    }
}

#[derive(Clone)]
pub struct make_C;

/// To be called by user directly setting up a client. Avoids
/// needing ClientFactory trait in scope, avoids unidiomatic
/// make_Trait name.
///
/// ```
/// # const _: &str = stringify! {
/// use bgs::client::BuckGraphService;
///
/// let protocol = BinaryProtocol::new();
/// let transport = HttpClient::new();
/// let client = <dyn BuckGraphService>::new(protocol, transport);
/// # };
/// ```
impl dyn C {
    pub fn new<P, T>(
        protocol: P,
        transport: T,
    ) -> ::std::sync::Arc<impl C + ::std::marker::Send + ::std::marker::Sync + 'static>
    where
        P: ::fbthrift::Protocol<Frame = T>,
        T: ::fbthrift::Transport,
        P::Deserializer: ::std::marker::Send,
    {
        let spawner = ::fbthrift::help::NoopSpawner;
        Self::with_spawner(protocol, transport, spawner)
    }

    pub fn with_spawner<P, T, S>(
        protocol: P,
        transport: T,
        spawner: S,
    ) -> ::std::sync::Arc<impl C + ::std::marker::Send + ::std::marker::Sync + 'static>
    where
        P: ::fbthrift::Protocol<Frame = T>,
        T: ::fbthrift::Transport,
        P::Deserializer: ::std::marker::Send,
        S: ::fbthrift::help::Spawner,
    {
        let _ = protocol;
        let _ = spawner;
        ::std::sync::Arc::new(CImpl::<P, T, S>::new(transport))
    }
}

impl<T> dyn CExt<T>
where
    T: ::fbthrift::Transport,
{
    pub fn new<P>(
        protocol: P,
        transport: T,
    ) -> ::std::sync::Arc<impl CExt<T> + ::std::marker::Send + ::std::marker::Sync + 'static>
    where
        P: ::fbthrift::Protocol<Frame = T>,
        P::Deserializer: ::std::marker::Send,
    {
        let spawner = ::fbthrift::help::NoopSpawner;
        Self::with_spawner(protocol, transport, spawner)
    }

    pub fn with_spawner<P, S>(
        protocol: P,
        transport: T,
        spawner: S,
    ) -> ::std::sync::Arc<impl CExt<T> + ::std::marker::Send + ::std::marker::Sync + 'static>
    where
        P: ::fbthrift::Protocol<Frame = T>,
        P::Deserializer: ::std::marker::Send,
        S: ::fbthrift::help::Spawner,
    {
        let _ = protocol;
        let _ = spawner;
        ::std::sync::Arc::new(CImpl::<P, T, S>::new(transport))
    }
}

pub type CDynClient = <make_C as ::fbthrift::ClientFactory>::Api;
pub type CClient = ::std::sync::Arc<CDynClient>;

/// The same thing, but to be called from generic contexts where we are
/// working with a type parameter `C: ClientFactory` to produce clients.
impl ::fbthrift::ClientFactory for make_C {
    type Api = dyn C + ::std::marker::Send + ::std::marker::Sync + 'static;

    fn with_spawner<P, T, S>(protocol: P, transport: T, spawner: S) -> ::std::sync::Arc<Self::Api>
    where
        P: ::fbthrift::Protocol<Frame = T>,
        T: ::fbthrift::Transport,
        P::Deserializer: ::std::marker::Send,
        S: ::fbthrift::help::Spawner,
    {
        <dyn C>::with_spawner(protocol, transport, spawner)
    }
}

