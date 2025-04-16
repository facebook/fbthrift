// @generated by Thrift for thrift/compiler/test/fixtures/rust-raw-identifiers/src/mod.thrift
// This file is probably not the place you want to edit!


#![recursion_limit = "100000000"]
#![allow(non_camel_case_types, non_snake_case, non_upper_case_globals, unused_crate_dependencies, clippy::redundant_closure, clippy::type_complexity)]

pub mod services;

#[allow(unused_imports)]
pub(crate) use crate as types;

#[derive(Clone, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct ThereAreNoPascalCaseKeywords {
    pub r#return: ::std::primitive::bool,
    pub super_: ::std::primitive::bool,
    // This field forces `..Default::default()` when instantiating this
    // struct, to make code future-proof against new fields added later to
    // the definition in Thrift. If you don't want this, add the annotation
    // `@rust.Exhaustive` to the Thrift struct to eliminate this field.
    #[doc(hidden)]
    pub _dot_dot_Default_default: self::dot_dot::OtherFields,
}



#[allow(clippy::derivable_impls)]
impl ::std::default::Default for self::ThereAreNoPascalCaseKeywords {
    fn default() -> Self {
        Self {
            r#return: ::std::default::Default::default(),
            super_: ::std::default::Default::default(),
            _dot_dot_Default_default: self::dot_dot::OtherFields(()),
        }
    }
}

impl ::std::fmt::Debug for self::ThereAreNoPascalCaseKeywords {
    fn fmt(&self, formatter: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        formatter
            .debug_struct("ThereAreNoPascalCaseKeywords")
            .field("r#return", &self.r#return)
            .field("super_", &self.super_)
            .finish()
    }
}

unsafe impl ::std::marker::Send for self::ThereAreNoPascalCaseKeywords {}
unsafe impl ::std::marker::Sync for self::ThereAreNoPascalCaseKeywords {}
impl ::std::marker::Unpin for self::ThereAreNoPascalCaseKeywords {}
impl ::std::panic::RefUnwindSafe for self::ThereAreNoPascalCaseKeywords {}
impl ::std::panic::UnwindSafe for self::ThereAreNoPascalCaseKeywords {}

impl ::fbthrift::GetTType for self::ThereAreNoPascalCaseKeywords {
    const TTYPE: ::fbthrift::TType = ::fbthrift::TType::Struct;
}

impl ::fbthrift::GetTypeNameType for self::ThereAreNoPascalCaseKeywords {
    fn type_name_type() -> fbthrift::TypeNameType {
        ::fbthrift::TypeNameType::StructType
    }
}

impl<P> ::fbthrift::Serialize<P> for self::ThereAreNoPascalCaseKeywords
where
    P: ::fbthrift::ProtocolWriter,
{
    #[inline]
    fn rs_thrift_write(&self, p: &mut P) {
        p.write_struct_begin("ThereAreNoPascalCaseKeywords");
        p.write_field_begin("return", ::fbthrift::TType::Bool, 1);
        ::fbthrift::Serialize::rs_thrift_write(&self.r#return, p);
        p.write_field_end();
        p.write_field_begin("super", ::fbthrift::TType::Bool, 2);
        ::fbthrift::Serialize::rs_thrift_write(&self.super_, p);
        p.write_field_end();
        p.write_field_stop();
        p.write_struct_end();
    }
}

impl<P> ::fbthrift::Deserialize<P> for self::ThereAreNoPascalCaseKeywords
where
    P: ::fbthrift::ProtocolReader,
{
    #[inline]
    fn rs_thrift_read(p: &mut P) -> ::anyhow::Result<Self> {
        static FIELDS: &[::fbthrift::Field] = &[
            ::fbthrift::Field::new("return", ::fbthrift::TType::Bool, 1),
            ::fbthrift::Field::new("super", ::fbthrift::TType::Bool, 2),
        ];
        let mut field_return = ::std::option::Option::None;
        let mut field_super = ::std::option::Option::None;
        let _ = ::anyhow::Context::context(p.read_struct_begin(|_| ()), "Expected a ThereAreNoPascalCaseKeywords")?;
        loop {
            let (_, fty, fid) = p.read_field_begin(|_| (), FIELDS)?;
            match (fty, fid as ::std::primitive::i32) {
                (::fbthrift::TType::Stop, _) => break,
                (::fbthrift::TType::Bool, 1) => field_return = ::std::option::Option::Some(::anyhow::Context::context(::fbthrift::Deserialize::rs_thrift_read(p), ::fbthrift::errors::DeserializingFieldError { field: "return", strct: "ThereAreNoPascalCaseKeywords"})?),
                (::fbthrift::TType::Bool, 2) => field_super = ::std::option::Option::Some(::anyhow::Context::context(::fbthrift::Deserialize::rs_thrift_read(p), ::fbthrift::errors::DeserializingFieldError { field: "super", strct: "ThereAreNoPascalCaseKeywords"})?),
                (fty, _) => p.skip(fty)?,
            }
            p.read_field_end()?;
        }
        p.read_struct_end()?;
        ::std::result::Result::Ok(Self {
            r#return: field_return.unwrap_or_default(),
            super_: field_super.unwrap_or_default(),
            _dot_dot_Default_default: self::dot_dot::OtherFields(()),
        })
    }
}


impl ::fbthrift::metadata::ThriftAnnotations for ThereAreNoPascalCaseKeywords {
    fn get_structured_annotation<T: Sized + 'static>() -> ::std::option::Option<T> {
        #[allow(unused_variables)]
        let type_id = ::std::any::TypeId::of::<T>();

        ::std::option::Option::None
    }

    fn get_field_structured_annotation<T: Sized + 'static>(field_id: ::std::primitive::i16) -> ::std::option::Option<T> {
        #[allow(unused_variables)]
        let type_id = ::std::any::TypeId::of::<T>();

        #[allow(clippy::match_single_binding)]
        match field_id {
            1 => {
            },
            2 => {

                if type_id == ::std::any::TypeId::of::<rust__types::Name>() {
                    let mut tmp = ::std::option::Option::Some(rust__types::Name {
                        name: "super_".to_owned(),
                        ..::std::default::Default::default()
                    });
                    let r: &mut dyn ::std::any::Any = &mut tmp;
                    let r: &mut ::std::option::Option<T> = r.downcast_mut().unwrap();
                    return r.take();
                }

                if let ::std::option::Option::Some(r) = <rust__types::Name as ::fbthrift::metadata::ThriftAnnotations>::get_structured_annotation::<T>() {
                    return ::std::option::Option::Some(r);
                }
            },
            _ => {}
        }

        ::std::option::Option::None
    }
}



mod dot_dot {
    #[derive(Copy, Clone, PartialEq, Eq, PartialOrd, Ord, Hash)]
    pub struct OtherFields(pub(crate) ());

    #[allow(dead_code)] // if serde isn't being used
    pub(super) fn default_for_serde_deserialize() -> OtherFields {
        OtherFields(())
    }
}

pub(crate) mod r#impl {
    use ::ref_cast::RefCast;

    #[derive(RefCast)]
    #[repr(transparent)]
    pub(crate) struct LocalImpl<T>(T);

    #[allow(unused)]
    pub(crate) fn rs_thrift_write<T, P>(value: &T, p: &mut P)
    where
        LocalImpl<T>: ::fbthrift::Serialize<P>,
        P: ::fbthrift::ProtocolWriter,
    {
        ::fbthrift::Serialize::rs_thrift_write(LocalImpl::ref_cast(value), p);
    }

    #[allow(unused)]
    pub(crate) fn rs_thrift_read<T, P>(p: &mut P) -> ::anyhow::Result<T>
    where
        LocalImpl<T>: ::fbthrift::Deserialize<P>,
        P: ::fbthrift::ProtocolReader,
    {
        let value: LocalImpl<T> = ::fbthrift::Deserialize::rs_thrift_read(p)?;
        ::std::result::Result::Ok(value.0)
    }
}


#[doc(hidden)]
#[deprecated]
#[allow(hidden_glob_reexports)]
pub mod __constructors {
}
