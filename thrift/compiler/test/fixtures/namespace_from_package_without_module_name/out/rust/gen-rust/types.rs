// @generated by Thrift for thrift/compiler/test/fixtures/namespace_from_package_without_module_name/src/module.thrift
// This file is probably not the place you want to edit!


#![recursion_limit = "100000000"]
#![allow(non_camel_case_types, non_snake_case, non_upper_case_globals, unused_crate_dependencies, clippy::redundant_closure, clippy::type_complexity)]

pub mod services;

#[allow(unused_imports)]
pub(crate) use crate as types;

#[derive(Clone, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct Foo {
    pub MyInt: ::std::primitive::i64,
    // This field forces `..Default::default()` when instantiating this
    // struct, to make code future-proof against new fields added later to
    // the definition in Thrift. If you don't want this, add the annotation
    // `@rust.Exhaustive` to the Thrift struct to eliminate this field.
    #[doc(hidden)]
    pub _dot_dot_Default_default: self::dot_dot::OtherFields,
}



#[allow(clippy::derivable_impls)]
impl ::std::default::Default for self::Foo {
    fn default() -> Self {
        Self {
            MyInt: ::std::default::Default::default(),
            _dot_dot_Default_default: self::dot_dot::OtherFields(()),
        }
    }
}

impl ::std::fmt::Debug for self::Foo {
    fn fmt(&self, formatter: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        formatter
            .debug_struct("Foo")
            .field("MyInt", &self.MyInt)
            .finish()
    }
}

unsafe impl ::std::marker::Send for self::Foo {}
unsafe impl ::std::marker::Sync for self::Foo {}
impl ::std::marker::Unpin for self::Foo {}
impl ::std::panic::RefUnwindSafe for self::Foo {}
impl ::std::panic::UnwindSafe for self::Foo {}

impl ::fbthrift::GetTType for self::Foo {
    const TTYPE: ::fbthrift::TType = ::fbthrift::TType::Struct;
}

impl ::fbthrift::GetUri for self::Foo {
    fn uri() -> &'static ::std::primitive::str {
        "test.dev/namespace_from_package_without_module_name/Foo"
    }
}

impl ::fbthrift::GetTypeNameType for self::Foo {
    fn type_name_type() -> fbthrift::TypeNameType {
        ::fbthrift::TypeNameType::StructType
    }
}

impl<P> ::fbthrift::Serialize<P> for self::Foo
where
    P: ::fbthrift::ProtocolWriter,
{
    #[inline]
    fn write(&self, p: &mut P) {
        p.write_struct_begin("Foo");
        p.write_field_begin("MyInt", ::fbthrift::TType::I64, 1);
        ::fbthrift::Serialize::write(&self.MyInt, p);
        p.write_field_end();
        p.write_field_stop();
        p.write_struct_end();
    }
}

impl<P> ::fbthrift::Deserialize<P> for self::Foo
where
    P: ::fbthrift::ProtocolReader,
{
    #[inline]
    fn read(p: &mut P) -> ::anyhow::Result<Self> {
        static FIELDS: &[::fbthrift::Field] = &[
            ::fbthrift::Field::new("MyInt", ::fbthrift::TType::I64, 1),
        ];
        let mut field_MyInt = ::std::option::Option::None;
        let _ = ::anyhow::Context::context(p.read_struct_begin(|_| ()), "Expected a Foo")?;
        loop {
            let (_, fty, fid) = p.read_field_begin(|_| (), FIELDS)?;
            match (fty, fid as ::std::primitive::i32) {
                (::fbthrift::TType::Stop, _) => break,
                (::fbthrift::TType::I64, 1) => field_MyInt = ::std::option::Option::Some(::anyhow::Context::context(::fbthrift::Deserialize::read(p), ::fbthrift::errors::DeserializingFieldError { field: "MyInt", strct: "Foo"})?),
                (fty, _) => p.skip(fty)?,
            }
            p.read_field_end()?;
        }
        p.read_struct_end()?;
        ::std::result::Result::Ok(Self {
            MyInt: field_MyInt.unwrap_or_default(),
            _dot_dot_Default_default: self::dot_dot::OtherFields(()),
        })
    }
}


impl ::fbthrift::metadata::ThriftAnnotations for Foo {
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
    pub(crate) fn write<T, P>(value: &T, p: &mut P)
    where
        LocalImpl<T>: ::fbthrift::Serialize<P>,
        P: ::fbthrift::ProtocolWriter,
    {
        ::fbthrift::Serialize::write(LocalImpl::ref_cast(value), p);
    }

    #[allow(unused)]
    pub(crate) fn read<T, P>(p: &mut P) -> ::anyhow::Result<T>
    where
        LocalImpl<T>: ::fbthrift::Deserialize<P>,
        P: ::fbthrift::ProtocolReader,
    {
        let value: LocalImpl<T> = ::fbthrift::Deserialize::read(p)?;
        ::std::result::Result::Ok(value.0)
    }
}


#[doc(hidden)]
#[deprecated]
#[allow(hidden_glob_reexports)]
pub mod __constructors {
}
