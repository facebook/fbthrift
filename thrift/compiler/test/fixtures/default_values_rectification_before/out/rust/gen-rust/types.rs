// @generated by Thrift for thrift/compiler/test/fixtures/default_values_rectification_before/src/module.thrift
// This file is probably not the place you want to edit!


#![recursion_limit = "100000000"]
#![allow(non_camel_case_types, non_snake_case, non_upper_case_globals, unused_crate_dependencies, clippy::redundant_closure, clippy::type_complexity)]

#[allow(unused_imports)]
pub(crate) use crate as types;

#[derive(Clone, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct EmptyStruct {
    // This field forces `..Default::default()` when instantiating this
    // struct, to make code future-proof against new fields added later to
    // the definition in Thrift. If you don't want this, add the annotation
    // `@rust.Exhaustive` to the Thrift struct to eliminate this field.
    #[doc(hidden)]
    pub _dot_dot_Default_default: self::dot_dot::OtherFields,
}

#[derive(Clone, PartialEq)]
pub struct TestStruct {
    pub unqualified_int_field: ::std::primitive::i32,
    pub unqualified_bool_field: ::std::primitive::bool,
    pub unqualified_list_field: ::std::vec::Vec<::std::primitive::i32>,
    pub unqualified_struct_field: crate::types::EmptyStruct,
    pub optional_int_field: ::std::option::Option<::std::primitive::i32>,
    pub optional_bool_field: ::std::option::Option<::std::primitive::bool>,
    pub optional_list_field: ::std::option::Option<::std::vec::Vec<::std::primitive::i32>>,
    pub optional_struct_field: ::std::option::Option<crate::types::EmptyStruct>,
    // This field forces `..Default::default()` when instantiating this
    // struct, to make code future-proof against new fields added later to
    // the definition in Thrift. If you don't want this, add the annotation
    // `@rust.Exhaustive` to the Thrift struct to eliminate this field.
    #[doc(hidden)]
    pub _dot_dot_Default_default: self::dot_dot::OtherFields,
}



#[allow(clippy::derivable_impls)]
impl ::std::default::Default for self::EmptyStruct {
    fn default() -> Self {
        Self {
            _dot_dot_Default_default: self::dot_dot::OtherFields(()),
        }
    }
}

impl ::std::fmt::Debug for self::EmptyStruct {
    fn fmt(&self, formatter: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        formatter
            .debug_struct("EmptyStruct")
            .finish()
    }
}

unsafe impl ::std::marker::Send for self::EmptyStruct {}
unsafe impl ::std::marker::Sync for self::EmptyStruct {}
impl ::std::marker::Unpin for self::EmptyStruct {}
impl ::std::panic::RefUnwindSafe for self::EmptyStruct {}
impl ::std::panic::UnwindSafe for self::EmptyStruct {}

impl ::fbthrift::GetTType for self::EmptyStruct {
    const TTYPE: ::fbthrift::TType = ::fbthrift::TType::Struct;
}

impl ::fbthrift::GetUri for self::EmptyStruct {
    fn uri() -> &'static ::std::primitive::str {
        "facebook.com/thrift/compiler/test/fixtures/default_values_rectification/EmptyStruct"
    }
}

impl ::fbthrift::GetTypeNameType for self::EmptyStruct {
    fn type_name_type() -> fbthrift::TypeNameType {
        ::fbthrift::TypeNameType::StructType
    }
}

impl<P> ::fbthrift::Serialize<P> for self::EmptyStruct
where
    P: ::fbthrift::ProtocolWriter,
{
    #[inline]
    fn write(&self, p: &mut P) {
        p.write_struct_begin("EmptyStruct");
        p.write_field_stop();
        p.write_struct_end();
    }
}

impl<P> ::fbthrift::Deserialize<P> for self::EmptyStruct
where
    P: ::fbthrift::ProtocolReader,
{
    #[inline]
    fn read(p: &mut P) -> ::anyhow::Result<Self> {
        static FIELDS: &[::fbthrift::Field] = &[
        ];
        #[allow(unused_mut)]
        let mut fields = Self {
            _dot_dot_Default_default: self::dot_dot::OtherFields(()),
        };
        let _ = ::anyhow::Context::context(p.read_struct_begin(|_| ()), "Expected a EmptyStruct")?;
        loop {
            let (_, fty, fid) = p.read_field_begin(|_| (), FIELDS)?;
            match (fty, fid as ::std::primitive::i32) {
                (::fbthrift::TType::Stop, _) => break,
                (fty, _) => p.skip(fty)?,
            }
            p.read_field_end()?;
        }
        p.read_struct_end()?;
        ::std::result::Result::Ok(fields)
    }
}


impl ::fbthrift::metadata::ThriftAnnotations for EmptyStruct {
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
            _ => {}
        }

        ::std::option::Option::None
    }
}


#[allow(clippy::derivable_impls)]
impl ::std::default::Default for self::TestStruct {
    fn default() -> Self {
        Self {
            unqualified_int_field: 0,
            unqualified_bool_field: false,
            unqualified_list_field: ::std::vec::Vec::new(),
            unqualified_struct_field: crate::types::EmptyStruct {
                    ..::std::default::Default::default()
                },
            optional_int_field: ::std::option::Option::None,
            optional_bool_field: ::std::option::Option::None,
            optional_list_field: ::std::option::Option::None,
            optional_struct_field: ::std::option::Option::None,
            _dot_dot_Default_default: self::dot_dot::OtherFields(()),
        }
    }
}

impl ::std::fmt::Debug for self::TestStruct {
    fn fmt(&self, formatter: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        formatter
            .debug_struct("TestStruct")
            .field("unqualified_int_field", &self.unqualified_int_field)
            .field("unqualified_bool_field", &self.unqualified_bool_field)
            .field("unqualified_list_field", &self.unqualified_list_field)
            .field("unqualified_struct_field", &self.unqualified_struct_field)
            .field("optional_int_field", &self.optional_int_field)
            .field("optional_bool_field", &self.optional_bool_field)
            .field("optional_list_field", &self.optional_list_field)
            .field("optional_struct_field", &self.optional_struct_field)
            .finish()
    }
}

unsafe impl ::std::marker::Send for self::TestStruct {}
unsafe impl ::std::marker::Sync for self::TestStruct {}
impl ::std::marker::Unpin for self::TestStruct {}
impl ::std::panic::RefUnwindSafe for self::TestStruct {}
impl ::std::panic::UnwindSafe for self::TestStruct {}

impl ::fbthrift::GetTType for self::TestStruct {
    const TTYPE: ::fbthrift::TType = ::fbthrift::TType::Struct;
}

impl ::fbthrift::GetUri for self::TestStruct {
    fn uri() -> &'static ::std::primitive::str {
        "facebook.com/thrift/compiler/test/fixtures/default_values_rectification/TestStruct"
    }
}

impl ::fbthrift::GetTypeNameType for self::TestStruct {
    fn type_name_type() -> fbthrift::TypeNameType {
        ::fbthrift::TypeNameType::StructType
    }
}

impl<P> ::fbthrift::Serialize<P> for self::TestStruct
where
    P: ::fbthrift::ProtocolWriter,
{
    #[inline]
    fn write(&self, p: &mut P) {
        p.write_struct_begin("TestStruct");
        p.write_field_begin("unqualified_int_field", ::fbthrift::TType::I32, 1);
        ::fbthrift::Serialize::write(&self.unqualified_int_field, p);
        p.write_field_end();
        p.write_field_begin("unqualified_bool_field", ::fbthrift::TType::Bool, 2);
        ::fbthrift::Serialize::write(&self.unqualified_bool_field, p);
        p.write_field_end();
        p.write_field_begin("unqualified_list_field", ::fbthrift::TType::List, 3);
        ::fbthrift::Serialize::write(&self.unqualified_list_field, p);
        p.write_field_end();
        p.write_field_begin("unqualified_struct_field", ::fbthrift::TType::Struct, 4);
        ::fbthrift::Serialize::write(&self.unqualified_struct_field, p);
        p.write_field_end();
        if let ::std::option::Option::Some(some) = &self.optional_int_field {
            p.write_field_begin("optional_int_field", ::fbthrift::TType::I32, 5);
            ::fbthrift::Serialize::write(some, p);
            p.write_field_end();
        }
        if let ::std::option::Option::Some(some) = &self.optional_bool_field {
            p.write_field_begin("optional_bool_field", ::fbthrift::TType::Bool, 6);
            ::fbthrift::Serialize::write(some, p);
            p.write_field_end();
        }
        if let ::std::option::Option::Some(some) = &self.optional_list_field {
            p.write_field_begin("optional_list_field", ::fbthrift::TType::List, 7);
            ::fbthrift::Serialize::write(some, p);
            p.write_field_end();
        }
        if let ::std::option::Option::Some(some) = &self.optional_struct_field {
            p.write_field_begin("optional_struct_field", ::fbthrift::TType::Struct, 8);
            ::fbthrift::Serialize::write(some, p);
            p.write_field_end();
        }
        p.write_field_stop();
        p.write_struct_end();
    }
}

impl<P> ::fbthrift::Deserialize<P> for self::TestStruct
where
    P: ::fbthrift::ProtocolReader,
{
    #[inline]
    fn read(p: &mut P) -> ::anyhow::Result<Self> {
        static FIELDS: &[::fbthrift::Field] = &[
            ::fbthrift::Field::new("optional_bool_field", ::fbthrift::TType::Bool, 6),
            ::fbthrift::Field::new("optional_int_field", ::fbthrift::TType::I32, 5),
            ::fbthrift::Field::new("optional_list_field", ::fbthrift::TType::List, 7),
            ::fbthrift::Field::new("optional_struct_field", ::fbthrift::TType::Struct, 8),
            ::fbthrift::Field::new("unqualified_bool_field", ::fbthrift::TType::Bool, 2),
            ::fbthrift::Field::new("unqualified_int_field", ::fbthrift::TType::I32, 1),
            ::fbthrift::Field::new("unqualified_list_field", ::fbthrift::TType::List, 3),
            ::fbthrift::Field::new("unqualified_struct_field", ::fbthrift::TType::Struct, 4),
        ];
        let mut field_unqualified_int_field = ::std::option::Option::None;
        let mut field_unqualified_bool_field = ::std::option::Option::None;
        let mut field_unqualified_list_field = ::std::option::Option::None;
        let mut field_unqualified_struct_field = ::std::option::Option::None;
        let mut field_optional_int_field = ::std::option::Option::None;
        let mut field_optional_bool_field = ::std::option::Option::None;
        let mut field_optional_list_field = ::std::option::Option::None;
        let mut field_optional_struct_field = ::std::option::Option::None;
        let _ = ::anyhow::Context::context(p.read_struct_begin(|_| ()), "Expected a TestStruct")?;
        loop {
            let (_, fty, fid) = p.read_field_begin(|_| (), FIELDS)?;
            match (fty, fid as ::std::primitive::i32) {
                (::fbthrift::TType::Stop, _) => break,
                (::fbthrift::TType::I32, 1) => field_unqualified_int_field = ::std::option::Option::Some(::fbthrift::Deserialize::read(p)?),
                (::fbthrift::TType::Bool, 2) => field_unqualified_bool_field = ::std::option::Option::Some(::fbthrift::Deserialize::read(p)?),
                (::fbthrift::TType::List, 3) => field_unqualified_list_field = ::std::option::Option::Some(::fbthrift::Deserialize::read(p)?),
                (::fbthrift::TType::Struct, 4) => field_unqualified_struct_field = ::std::option::Option::Some(::fbthrift::Deserialize::read(p)?),
                (::fbthrift::TType::I32, 5) => field_optional_int_field = ::std::option::Option::Some(::fbthrift::Deserialize::read(p)?),
                (::fbthrift::TType::Bool, 6) => field_optional_bool_field = ::std::option::Option::Some(::fbthrift::Deserialize::read(p)?),
                (::fbthrift::TType::List, 7) => field_optional_list_field = ::std::option::Option::Some(::fbthrift::Deserialize::read(p)?),
                (::fbthrift::TType::Struct, 8) => field_optional_struct_field = ::std::option::Option::Some(::fbthrift::Deserialize::read(p)?),
                (fty, _) => p.skip(fty)?,
            }
            p.read_field_end()?;
        }
        p.read_struct_end()?;
        ::std::result::Result::Ok(Self {
            unqualified_int_field: field_unqualified_int_field.unwrap_or(0),
            unqualified_bool_field: field_unqualified_bool_field.unwrap_or(false),
            unqualified_list_field: field_unqualified_list_field.unwrap_or_else(|| ::std::vec::Vec::new()),
            unqualified_struct_field: field_unqualified_struct_field.unwrap_or_else(|| crate::types::EmptyStruct {
                    ..::std::default::Default::default()
                }),
            optional_int_field: field_optional_int_field,
            optional_bool_field: field_optional_bool_field,
            optional_list_field: field_optional_list_field,
            optional_struct_field: field_optional_struct_field,
            _dot_dot_Default_default: self::dot_dot::OtherFields(()),
        })
    }
}


impl ::fbthrift::metadata::ThriftAnnotations for TestStruct {
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
            },
            3 => {
            },
            4 => {
            },
            5 => {
            },
            6 => {
            },
            7 => {
            },
            8 => {
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

