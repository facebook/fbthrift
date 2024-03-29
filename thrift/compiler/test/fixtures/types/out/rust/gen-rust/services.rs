// @generated by Thrift for thrift/compiler/test/fixtures/types/src/module.thrift
// This file is probably not the place you want to edit!

//! Thrift service definitions for `module`.


/// Service definitions for `SomeService`.
pub mod some_service {
    #[derive(Clone, Debug)]
    pub enum BounceMapExn {
        #[doc(hidden)]
        Success(included__types::SomeMap),
        ApplicationException(::fbthrift::ApplicationException),
    }

    impl ::std::convert::From<crate::errors::some_service::BounceMapError> for BounceMapExn {
        fn from(err: crate::errors::some_service::BounceMapError) -> Self {
            match err {
                crate::errors::some_service::BounceMapError::ApplicationException(aexn) => BounceMapExn::ApplicationException(aexn),
                crate::errors::some_service::BounceMapError::ThriftError(err) => BounceMapExn::ApplicationException(::fbthrift::ApplicationException {
                    message: err.to_string(),
                    type_: ::fbthrift::ApplicationExceptionErrorCode::InternalError,
                }),
            }
        }
    }

    impl ::std::convert::From<::fbthrift::ApplicationException> for BounceMapExn {
        fn from(exn: ::fbthrift::ApplicationException) -> Self {
            Self::ApplicationException(exn)
        }
    }

    impl ::fbthrift::ExceptionInfo for BounceMapExn {
        fn exn_name(&self) -> &'static str {
            match self {
                Self::Success(_) => panic!("ExceptionInfo::exn_name called on Success"),
                Self::ApplicationException(aexn) => aexn.exn_name(),
            }
        }

        fn exn_value(&self) -> String {
            match self {
                Self::Success(_) => panic!("ExceptionInfo::exn_value called on Success"),
                Self::ApplicationException(aexn) => aexn.exn_value(),
            }
        }

        fn exn_is_declared(&self) -> bool {
            match self {
                Self::Success(_) => panic!("ExceptionInfo::exn_is_declared called on Success"),
                Self::ApplicationException(aexn) => aexn.exn_is_declared(),
            }
        }
    }

    impl ::fbthrift::ResultInfo for BounceMapExn {
        fn result_type(&self) -> ::fbthrift::ResultType {
            match self {
                Self::Success(_) => ::fbthrift::ResultType::Return,
                Self::ApplicationException(_aexn) => ::fbthrift::ResultType::Exception,
            }
        }
    }

    impl ::fbthrift::GetTType for BounceMapExn {
        const TTYPE: ::fbthrift::TType = ::fbthrift::TType::Struct;
    }

    impl<P> ::fbthrift::Serialize<P> for BounceMapExn
    where
        P: ::fbthrift::ProtocolWriter,
    {
        fn write(&self, p: &mut P) {
            if let Self::ApplicationException(aexn) = self {
                return aexn.write(p);
            }
            p.write_struct_begin("BounceMap");
            match self {
                Self::Success(inner) => {
                    p.write_field_begin(
                        "Success",
                        ::fbthrift::TType::Map,
                        0i16,
                    );
                    inner.write(p);
                    p.write_field_end();
                }
                Self::ApplicationException(_aexn) => unreachable!(),
            }
            p.write_field_stop();
            p.write_struct_end();
        }
    }

    impl<P> ::fbthrift::Deserialize<P> for BounceMapExn
    where
        P: ::fbthrift::ProtocolReader,
    {
        fn read(p: &mut P) -> ::anyhow::Result<Self> {
            static RETURNS: &[::fbthrift::Field] = &[
                ::fbthrift::Field::new("Success", ::fbthrift::TType::Map, 0),
            ];
            let _ = p.read_struct_begin(|_| ())?;
            let mut once = false;
            let mut alt = ::std::option::Option::None;
            loop {
                let (_, fty, fid) = p.read_field_begin(|_| (), RETURNS)?;
                match ((fty, fid as ::std::primitive::i32), once) {
                    ((::fbthrift::TType::Stop, _), _) => {
                        p.read_field_end()?;
                        break;
                    }
                    ((::fbthrift::TType::Map, 0i32), false) => {
                        once = true;
                        alt = ::std::option::Option::Some(Self::Success(::fbthrift::Deserialize::read(p)?));
                    }
                    ((ty, _id), false) => p.skip(ty)?,
                    ((badty, badid), true) => return ::std::result::Result::Err(::std::convert::From::from(
                        ::fbthrift::ApplicationException::new(
                            ::fbthrift::ApplicationExceptionErrorCode::ProtocolError,
                            format!(
                                "unwanted extra union {} field ty {:?} id {}",
                                "BounceMapExn",
                                badty,
                                badid,
                            ),
                        )
                    )),
                }
                p.read_field_end()?;
            }
            p.read_struct_end()?;
            alt.ok_or_else(||
                ::fbthrift::ApplicationException::new(
                    ::fbthrift::ApplicationExceptionErrorCode::MissingResult,
                    format!("Empty union {}", "BounceMapExn"),
                )
                .into(),
            )
        }
    }

    #[derive(Clone, Debug)]
    pub enum BinaryKeyedMapExn {
        #[doc(hidden)]
        Success(::std::collections::BTreeMap<crate::types::TBinary, ::std::primitive::i64>),
        ApplicationException(::fbthrift::ApplicationException),
    }

    impl ::std::convert::From<crate::errors::some_service::BinaryKeyedMapError> for BinaryKeyedMapExn {
        fn from(err: crate::errors::some_service::BinaryKeyedMapError) -> Self {
            match err {
                crate::errors::some_service::BinaryKeyedMapError::ApplicationException(aexn) => BinaryKeyedMapExn::ApplicationException(aexn),
                crate::errors::some_service::BinaryKeyedMapError::ThriftError(err) => BinaryKeyedMapExn::ApplicationException(::fbthrift::ApplicationException {
                    message: err.to_string(),
                    type_: ::fbthrift::ApplicationExceptionErrorCode::InternalError,
                }),
            }
        }
    }

    impl ::std::convert::From<::fbthrift::ApplicationException> for BinaryKeyedMapExn {
        fn from(exn: ::fbthrift::ApplicationException) -> Self {
            Self::ApplicationException(exn)
        }
    }

    impl ::fbthrift::ExceptionInfo for BinaryKeyedMapExn {
        fn exn_name(&self) -> &'static str {
            match self {
                Self::Success(_) => panic!("ExceptionInfo::exn_name called on Success"),
                Self::ApplicationException(aexn) => aexn.exn_name(),
            }
        }

        fn exn_value(&self) -> String {
            match self {
                Self::Success(_) => panic!("ExceptionInfo::exn_value called on Success"),
                Self::ApplicationException(aexn) => aexn.exn_value(),
            }
        }

        fn exn_is_declared(&self) -> bool {
            match self {
                Self::Success(_) => panic!("ExceptionInfo::exn_is_declared called on Success"),
                Self::ApplicationException(aexn) => aexn.exn_is_declared(),
            }
        }
    }

    impl ::fbthrift::ResultInfo for BinaryKeyedMapExn {
        fn result_type(&self) -> ::fbthrift::ResultType {
            match self {
                Self::Success(_) => ::fbthrift::ResultType::Return,
                Self::ApplicationException(_aexn) => ::fbthrift::ResultType::Exception,
            }
        }
    }

    impl ::fbthrift::GetTType for BinaryKeyedMapExn {
        const TTYPE: ::fbthrift::TType = ::fbthrift::TType::Struct;
    }

    impl<P> ::fbthrift::Serialize<P> for BinaryKeyedMapExn
    where
        P: ::fbthrift::ProtocolWriter,
    {
        fn write(&self, p: &mut P) {
            if let Self::ApplicationException(aexn) = self {
                return aexn.write(p);
            }
            p.write_struct_begin("BinaryKeyedMap");
            match self {
                Self::Success(inner) => {
                    p.write_field_begin(
                        "Success",
                        ::fbthrift::TType::Map,
                        0i16,
                    );
                    inner.write(p);
                    p.write_field_end();
                }
                Self::ApplicationException(_aexn) => unreachable!(),
            }
            p.write_field_stop();
            p.write_struct_end();
        }
    }

    impl<P> ::fbthrift::Deserialize<P> for BinaryKeyedMapExn
    where
        P: ::fbthrift::ProtocolReader,
    {
        fn read(p: &mut P) -> ::anyhow::Result<Self> {
            static RETURNS: &[::fbthrift::Field] = &[
                ::fbthrift::Field::new("Success", ::fbthrift::TType::Map, 0),
            ];
            let _ = p.read_struct_begin(|_| ())?;
            let mut once = false;
            let mut alt = ::std::option::Option::None;
            loop {
                let (_, fty, fid) = p.read_field_begin(|_| (), RETURNS)?;
                match ((fty, fid as ::std::primitive::i32), once) {
                    ((::fbthrift::TType::Stop, _), _) => {
                        p.read_field_end()?;
                        break;
                    }
                    ((::fbthrift::TType::Map, 0i32), false) => {
                        once = true;
                        alt = ::std::option::Option::Some(Self::Success(::fbthrift::Deserialize::read(p)?));
                    }
                    ((ty, _id), false) => p.skip(ty)?,
                    ((badty, badid), true) => return ::std::result::Result::Err(::std::convert::From::from(
                        ::fbthrift::ApplicationException::new(
                            ::fbthrift::ApplicationExceptionErrorCode::ProtocolError,
                            format!(
                                "unwanted extra union {} field ty {:?} id {}",
                                "BinaryKeyedMapExn",
                                badty,
                                badid,
                            ),
                        )
                    )),
                }
                p.read_field_end()?;
            }
            p.read_struct_end()?;
            alt.ok_or_else(||
                ::fbthrift::ApplicationException::new(
                    ::fbthrift::ApplicationExceptionErrorCode::MissingResult,
                    format!("Empty union {}", "BinaryKeyedMapExn"),
                )
                .into(),
            )
        }
    }
}
