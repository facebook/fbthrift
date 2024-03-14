// @generated by Thrift for thrift/compiler/test/fixtures/req-opt/src/module.thrift
// This file is probably not the place you want to edit!


#![recursion_limit = "100000000"]
#![allow(non_camel_case_types, non_snake_case, non_upper_case_globals, unused_crate_dependencies, clippy::redundant_closure, clippy::type_complexity)]

pub mod errors;

pub use crate as types;

#[derive(Clone, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct Foo {
    pub myInteger: ::std::primitive::i32,
    pub myString: ::std::option::Option<::std::string::String>,
    pub myBools: ::std::vec::Vec<::std::primitive::bool>,
    pub myNumbers: ::std::vec::Vec<::std::primitive::i32>,
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
            myInteger: ::std::default::Default::default(),
            myString: ::std::option::Option::None,
            myBools: ::std::default::Default::default(),
            myNumbers: ::std::default::Default::default(),
            _dot_dot_Default_default: self::dot_dot::OtherFields(()),
        }
    }
}

impl ::std::fmt::Debug for self::Foo {
    fn fmt(&self, formatter: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        formatter
            .debug_struct("Foo")
            .field("myInteger", &self.myInteger)
            .field("myString", &self.myString)
            .field("myBools", &self.myBools)
            .field("myNumbers", &self.myNumbers)
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

impl<P> ::fbthrift::Serialize<P> for self::Foo
where
    P: ::fbthrift::ProtocolWriter,
{
    fn write(&self, p: &mut P) {
        p.write_struct_begin("Foo");
        p.write_field_begin("myInteger", ::fbthrift::TType::I32, 1);
        ::fbthrift::Serialize::write(&self.myInteger, p);
        p.write_field_end();
        if let ::std::option::Option::Some(some) = &self.myString {
            p.write_field_begin("myString", ::fbthrift::TType::String, 2);
            ::fbthrift::Serialize::write(some, p);
            p.write_field_end();
        }
        p.write_field_begin("myBools", ::fbthrift::TType::List, 3);
        ::fbthrift::Serialize::write(&self.myBools, p);
        p.write_field_end();
        p.write_field_begin("myNumbers", ::fbthrift::TType::List, 4);
        ::fbthrift::Serialize::write(&self.myNumbers, p);
        p.write_field_end();
        p.write_field_stop();
        p.write_struct_end();
    }
}

impl<P> ::fbthrift::Deserialize<P> for self::Foo
where
    P: ::fbthrift::ProtocolReader,
{
    fn read(p: &mut P) -> ::anyhow::Result<Self> {
        static FIELDS: &[::fbthrift::Field] = &[
            ::fbthrift::Field::new("myBools", ::fbthrift::TType::List, 3),
            ::fbthrift::Field::new("myInteger", ::fbthrift::TType::I32, 1),
            ::fbthrift::Field::new("myNumbers", ::fbthrift::TType::List, 4),
            ::fbthrift::Field::new("myString", ::fbthrift::TType::String, 2),
        ];
        let mut field_myInteger = ::std::option::Option::None;
        let mut field_myString = ::std::option::Option::None;
        let mut field_myBools = ::std::option::Option::None;
        let mut field_myNumbers = ::std::option::Option::None;
        let _ = ::anyhow::Context::context(p.read_struct_begin(|_| ()), "Expected a Foo")?;
        loop {
            let (_, fty, fid) = p.read_field_begin(|_| (), FIELDS)?;
            match (fty, fid as ::std::primitive::i32) {
                (::fbthrift::TType::Stop, _) => break,
                (::fbthrift::TType::I32, 1) => field_myInteger = ::std::option::Option::Some(::fbthrift::Deserialize::read(p)?),
                (::fbthrift::TType::String, 2) => field_myString = ::std::option::Option::Some(::fbthrift::Deserialize::read(p)?),
                (::fbthrift::TType::List, 3) => field_myBools = ::std::option::Option::Some(::fbthrift::Deserialize::read(p)?),
                (::fbthrift::TType::List, 4) => field_myNumbers = ::std::option::Option::Some(::fbthrift::Deserialize::read(p)?),
                (fty, _) => p.skip(fty)?,
            }
            p.read_field_end()?;
        }
        p.read_struct_end()?;
        ::std::result::Result::Ok(Self {
            myInteger: field_myInteger.unwrap_or_default(),
            myString: field_myString,
            myBools: field_myBools.unwrap_or_default(),
            myNumbers: field_myNumbers.unwrap_or_default(),
            _dot_dot_Default_default: self::dot_dot::OtherFields(()),
        })
    }
}


impl ::fbthrift::metadata::ThriftAnnotations for Foo {
    fn get_structured_annotation<T: Sized + 'static>() -> ::std::option::Option<T> {
        #[allow(unused_variables)]
        let type_id = ::std::any::TypeId::of::<T>();

        None
    }

    fn get_field_structured_annotation<T: Sized + 'static>(field_id: i16) -> ::std::option::Option<T> {
        #[allow(unused_variables)]
        let type_id = ::std::any::TypeId::of::<T>();

        #[allow(clippy::match_single_binding)]
        match field_id {
            1 => {
            },
            2 => {
            },
            3 => {
            },
            4 => {
            },
            _ => {}
        }

        None
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
    use ref_cast::RefCast;

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