// @generated by Thrift for thrift/compiler/test/fixtures/rust-skip-none-serialization/src/module.thrift
// This file is probably not the place you want to edit!


#![recursion_limit = "100000000"]
#![allow(non_camel_case_types, non_snake_case, non_upper_case_globals, unused_crate_dependencies, clippy::redundant_closure, clippy::type_complexity)]

extern crate serde;

pub mod services;

#[allow(unused_imports)]
pub(crate) use crate as types;

#[derive(Clone, PartialEq, Eq, PartialOrd, Ord, Hash, ::serde_derive::Serialize, ::serde_derive::Deserialize)]
pub struct MyStruct {
    #[serde(default)]
    pub MyIntField: ::std::primitive::i64,
    #[serde(default)]
    pub MyStringField: ::std::string::String,
    #[serde(skip_serializing_if = "::std::option::Option::is_none")]
    pub MyDataField: ::std::option::Option<::std::string::String>,
    // This field forces `..Default::default()` when instantiating this
    // struct, to make code future-proof against new fields added later to
    // the definition in Thrift. If you don't want this, add the annotation
    // `@rust.Exhaustive` to the Thrift struct to eliminate this field.
    #[doc(hidden)]
    #[serde(skip, default = "self::dot_dot::default_for_serde_deserialize")]
    pub _dot_dot_Default_default: self::dot_dot::OtherFields,
}



#[allow(clippy::derivable_impls)]
impl ::std::default::Default for self::MyStruct {
    fn default() -> Self {
        Self {
            MyIntField: ::std::default::Default::default(),
            MyStringField: ::std::default::Default::default(),
            MyDataField: ::std::option::Option::None,
            _dot_dot_Default_default: self::dot_dot::OtherFields(()),
        }
    }
}

impl ::std::fmt::Debug for self::MyStruct {
    fn fmt(&self, formatter: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        formatter
            .debug_struct("MyStruct")
            .field("MyIntField", &self.MyIntField)
            .field("MyStringField", &self.MyStringField)
            .field("MyDataField", &self.MyDataField)
            .finish()
    }
}

unsafe impl ::std::marker::Send for self::MyStruct {}
unsafe impl ::std::marker::Sync for self::MyStruct {}
impl ::std::marker::Unpin for self::MyStruct {}
impl ::std::panic::RefUnwindSafe for self::MyStruct {}
impl ::std::panic::UnwindSafe for self::MyStruct {}

impl ::fbthrift::GetTType for self::MyStruct {
    const TTYPE: ::fbthrift::TType = ::fbthrift::TType::Struct;
}

impl ::fbthrift::GetTypeNameType for self::MyStruct {
    fn type_name_type() -> fbthrift::TypeNameType {
        ::fbthrift::TypeNameType::StructType
    }
}

impl<P> ::fbthrift::Serialize<P> for self::MyStruct
where
    P: ::fbthrift::ProtocolWriter,
{
    #[inline]
    fn write(&self, p: &mut P) {
        p.write_struct_begin("MyStruct");
        p.write_field_begin("MyIntField", ::fbthrift::TType::I64, 1);
        ::fbthrift::Serialize::write(&self.MyIntField, p);
        p.write_field_end();
        p.write_field_begin("MyStringField", ::fbthrift::TType::String, 2);
        ::fbthrift::Serialize::write(&self.MyStringField, p);
        p.write_field_end();
        if let ::std::option::Option::Some(some) = &self.MyDataField {
            p.write_field_begin("MyDataField", ::fbthrift::TType::String, 3);
            ::fbthrift::Serialize::write(some, p);
            p.write_field_end();
        }
        p.write_field_stop();
        p.write_struct_end();
    }
}

impl<P> ::fbthrift::Deserialize<P> for self::MyStruct
where
    P: ::fbthrift::ProtocolReader,
{
    #[inline]
    fn read(p: &mut P) -> ::anyhow::Result<Self> {
        static FIELDS: &[::fbthrift::Field] = &[
            ::fbthrift::Field::new("MyDataField", ::fbthrift::TType::String, 3),
            ::fbthrift::Field::new("MyIntField", ::fbthrift::TType::I64, 1),
            ::fbthrift::Field::new("MyStringField", ::fbthrift::TType::String, 2),
        ];
        let mut field_MyIntField = ::std::option::Option::None;
        let mut field_MyStringField = ::std::option::Option::None;
        let mut field_MyDataField = ::std::option::Option::None;
        let _ = ::anyhow::Context::context(p.read_struct_begin(|_| ()), "Expected a MyStruct")?;
        loop {
            let (_, fty, fid) = p.read_field_begin(|_| (), FIELDS)?;
            match (fty, fid as ::std::primitive::i32) {
                (::fbthrift::TType::Stop, _) => break,
                (::fbthrift::TType::I64, 1) => field_MyIntField = ::std::option::Option::Some(::anyhow::Context::context(::fbthrift::Deserialize::read(p), "Error while deserialising MyIntField field of MyStruct")?),
                (::fbthrift::TType::String, 2) => field_MyStringField = ::std::option::Option::Some(::anyhow::Context::context(::fbthrift::Deserialize::read(p), "Error while deserialising MyStringField field of MyStruct")?),
                (::fbthrift::TType::String, 3) => field_MyDataField = ::std::option::Option::Some(::anyhow::Context::context(::fbthrift::Deserialize::read(p), "Error while deserialising MyDataField field of MyStruct")?),
                (fty, _) => p.skip(fty)?,
            }
            p.read_field_end()?;
        }
        p.read_struct_end()?;
        ::std::result::Result::Ok(Self {
            MyIntField: field_MyIntField.unwrap_or_default(),
            MyStringField: field_MyStringField.unwrap_or_default(),
            MyDataField: field_MyDataField,
            _dot_dot_Default_default: self::dot_dot::OtherFields(()),
        })
    }
}


impl ::fbthrift::metadata::ThriftAnnotations for MyStruct {
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
