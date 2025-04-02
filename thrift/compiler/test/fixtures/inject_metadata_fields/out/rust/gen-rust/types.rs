// @generated by Thrift for thrift/compiler/test/fixtures/inject_metadata_fields/src/module.thrift
// This file is probably not the place you want to edit!


#![recursion_limit = "100000000"]
#![allow(non_camel_case_types, non_snake_case, non_upper_case_globals, unused_crate_dependencies, clippy::redundant_closure, clippy::type_complexity)]

#[allow(unused_imports)]
pub(crate) use crate as types;

#[derive(Clone, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct Fields {
    pub injected_field: ::std::string::String,
    // This field forces `..Default::default()` when instantiating this
    // struct, to make code future-proof against new fields added later to
    // the definition in Thrift. If you don't want this, add the annotation
    // `@rust.Exhaustive` to the Thrift struct to eliminate this field.
    #[doc(hidden)]
    pub _dot_dot_Default_default: self::dot_dot::OtherFields,
}

#[derive(Clone, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct FieldsInjectedToEmptyStruct {
    pub injected_field: ::std::string::String,
    // This field forces `..Default::default()` when instantiating this
    // struct, to make code future-proof against new fields added later to
    // the definition in Thrift. If you don't want this, add the annotation
    // `@rust.Exhaustive` to the Thrift struct to eliminate this field.
    #[doc(hidden)]
    pub _dot_dot_Default_default: self::dot_dot::OtherFields,
}

#[derive(Clone, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct FieldsInjectedToStruct {
    pub string_field: ::std::string::String,
    pub injected_field: ::std::string::String,
    // This field forces `..Default::default()` when instantiating this
    // struct, to make code future-proof against new fields added later to
    // the definition in Thrift. If you don't want this, add the annotation
    // `@rust.Exhaustive` to the Thrift struct to eliminate this field.
    #[doc(hidden)]
    pub _dot_dot_Default_default: self::dot_dot::OtherFields,
}

#[derive(Clone, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct FieldsInjectedWithIncludedStruct {
    pub string_field: ::std::string::String,
    pub injected_field: ::std::string::String,
    pub injected_structured_annotation_field: ::std::option::Option<::std::boxed::Box<::std::string::String>>,
    pub injected_unstructured_annotation_field: ::std::option::Option<::std::boxed::Box<::std::string::String>>,
    // This field forces `..Default::default()` when instantiating this
    // struct, to make code future-proof against new fields added later to
    // the definition in Thrift. If you don't want this, add the annotation
    // `@rust.Exhaustive` to the Thrift struct to eliminate this field.
    #[doc(hidden)]
    pub _dot_dot_Default_default: self::dot_dot::OtherFields,
}



#[allow(clippy::derivable_impls)]
impl ::std::default::Default for self::Fields {
    fn default() -> Self {
        Self {
            injected_field: ::std::default::Default::default(),
            _dot_dot_Default_default: self::dot_dot::OtherFields(()),
        }
    }
}

impl ::std::fmt::Debug for self::Fields {
    fn fmt(&self, formatter: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        formatter
            .debug_struct("Fields")
            .field("injected_field", &self.injected_field)
            .finish()
    }
}

unsafe impl ::std::marker::Send for self::Fields {}
unsafe impl ::std::marker::Sync for self::Fields {}
impl ::std::marker::Unpin for self::Fields {}
impl ::std::panic::RefUnwindSafe for self::Fields {}
impl ::std::panic::UnwindSafe for self::Fields {}

impl ::fbthrift::GetTType for self::Fields {
    const TTYPE: ::fbthrift::TType = ::fbthrift::TType::Struct;
}

impl ::fbthrift::GetTypeNameType for self::Fields {
    fn type_name_type() -> fbthrift::TypeNameType {
        ::fbthrift::TypeNameType::StructType
    }
}

impl<P> ::fbthrift::Serialize<P> for self::Fields
where
    P: ::fbthrift::ProtocolWriter,
{
    #[inline]
    fn write(&self, p: &mut P) {
        p.write_struct_begin("Fields");
        p.write_field_begin("injected_field", ::fbthrift::TType::String, 100);
        ::fbthrift::Serialize::write(&self.injected_field, p);
        p.write_field_end();
        p.write_field_stop();
        p.write_struct_end();
    }
}

impl<P> ::fbthrift::Deserialize<P> for self::Fields
where
    P: ::fbthrift::ProtocolReader,
{
    #[inline]
    fn read(p: &mut P) -> ::anyhow::Result<Self> {
        static FIELDS: &[::fbthrift::Field] = &[
            ::fbthrift::Field::new("injected_field", ::fbthrift::TType::String, 100),
        ];
        let mut field_injected_field = ::std::option::Option::None;
        let _ = ::anyhow::Context::context(p.read_struct_begin(|_| ()), "Expected a Fields")?;
        loop {
            #![allow(unused_imports)]
            use ::anyhow::Context;
            let (_, fty, fid) = p.read_field_begin(|_| (), FIELDS)?;
            match (fty, fid as ::std::primitive::i32) {
                (::fbthrift::TType::Stop, _) => break,
                (::fbthrift::TType::String, 100) => field_injected_field = ::std::option::Option::Some(::fbthrift::Deserialize::read(p).with_context(||format!("Error while deserialising injected_field field of Fields"))?),
                (fty, _) => p.skip(fty)?,
            }
            p.read_field_end()?;
        }
        p.read_struct_end()?;
        ::std::result::Result::Ok(Self {
            injected_field: field_injected_field.unwrap_or_default(),
            _dot_dot_Default_default: self::dot_dot::OtherFields(()),
        })
    }
}


impl ::fbthrift::metadata::ThriftAnnotations for Fields {
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
            100 => {
            },
            _ => {}
        }

        ::std::option::Option::None
    }
}


#[allow(clippy::derivable_impls)]
impl ::std::default::Default for self::FieldsInjectedToEmptyStruct {
    fn default() -> Self {
        Self {
            injected_field: ::std::default::Default::default(),
            _dot_dot_Default_default: self::dot_dot::OtherFields(()),
        }
    }
}

impl ::std::fmt::Debug for self::FieldsInjectedToEmptyStruct {
    fn fmt(&self, formatter: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        formatter
            .debug_struct("FieldsInjectedToEmptyStruct")
            .field("injected_field", &self.injected_field)
            .finish()
    }
}

unsafe impl ::std::marker::Send for self::FieldsInjectedToEmptyStruct {}
unsafe impl ::std::marker::Sync for self::FieldsInjectedToEmptyStruct {}
impl ::std::marker::Unpin for self::FieldsInjectedToEmptyStruct {}
impl ::std::panic::RefUnwindSafe for self::FieldsInjectedToEmptyStruct {}
impl ::std::panic::UnwindSafe for self::FieldsInjectedToEmptyStruct {}

impl ::fbthrift::GetTType for self::FieldsInjectedToEmptyStruct {
    const TTYPE: ::fbthrift::TType = ::fbthrift::TType::Struct;
}

impl ::fbthrift::GetTypeNameType for self::FieldsInjectedToEmptyStruct {
    fn type_name_type() -> fbthrift::TypeNameType {
        ::fbthrift::TypeNameType::StructType
    }
}

impl<P> ::fbthrift::Serialize<P> for self::FieldsInjectedToEmptyStruct
where
    P: ::fbthrift::ProtocolWriter,
{
    #[inline]
    fn write(&self, p: &mut P) {
        p.write_struct_begin("FieldsInjectedToEmptyStruct");
        p.write_field_begin("injected_field", ::fbthrift::TType::String, -1100);
        ::fbthrift::Serialize::write(&self.injected_field, p);
        p.write_field_end();
        p.write_field_stop();
        p.write_struct_end();
    }
}

impl<P> ::fbthrift::Deserialize<P> for self::FieldsInjectedToEmptyStruct
where
    P: ::fbthrift::ProtocolReader,
{
    #[inline]
    fn read(p: &mut P) -> ::anyhow::Result<Self> {
        static FIELDS: &[::fbthrift::Field] = &[
            ::fbthrift::Field::new("injected_field", ::fbthrift::TType::String, -1100),
        ];
        let mut field_injected_field = ::std::option::Option::None;
        let _ = ::anyhow::Context::context(p.read_struct_begin(|_| ()), "Expected a FieldsInjectedToEmptyStruct")?;
        loop {
            #![allow(unused_imports)]
            use ::anyhow::Context;
            let (_, fty, fid) = p.read_field_begin(|_| (), FIELDS)?;
            match (fty, fid as ::std::primitive::i32) {
                (::fbthrift::TType::Stop, _) => break,
                (::fbthrift::TType::String, -1100) => field_injected_field = ::std::option::Option::Some(::fbthrift::Deserialize::read(p).with_context(||format!("Error while deserialising injected_field field of FieldsInjectedToEmptyStruct"))?),
                (fty, _) => p.skip(fty)?,
            }
            p.read_field_end()?;
        }
        p.read_struct_end()?;
        ::std::result::Result::Ok(Self {
            injected_field: field_injected_field.unwrap_or_default(),
            _dot_dot_Default_default: self::dot_dot::OtherFields(()),
        })
    }
}


impl ::fbthrift::metadata::ThriftAnnotations for FieldsInjectedToEmptyStruct {
    fn get_structured_annotation<T: Sized + 'static>() -> ::std::option::Option<T> {
        #[allow(unused_variables)]
        let type_id = ::std::any::TypeId::of::<T>();

        if type_id == ::std::any::TypeId::of::<internal__types::InjectMetadataFields>() {
            let mut tmp = ::std::option::Option::Some(internal__types::InjectMetadataFields {
                r#type: "Fields".to_owned(),
                ..::std::default::Default::default()
            });
            let r: &mut dyn ::std::any::Any = &mut tmp;
            let r: &mut ::std::option::Option<T> = r.downcast_mut().unwrap();
            return r.take();
        }

        ::std::option::Option::None
    }

    fn get_field_structured_annotation<T: Sized + 'static>(field_id: ::std::primitive::i16) -> ::std::option::Option<T> {
        #[allow(unused_variables)]
        let type_id = ::std::any::TypeId::of::<T>();

        #[allow(clippy::match_single_binding)]
        match field_id {
            -1100 => {
            },
            _ => {}
        }

        ::std::option::Option::None
    }
}


#[allow(clippy::derivable_impls)]
impl ::std::default::Default for self::FieldsInjectedToStruct {
    fn default() -> Self {
        Self {
            string_field: ::std::default::Default::default(),
            injected_field: ::std::default::Default::default(),
            _dot_dot_Default_default: self::dot_dot::OtherFields(()),
        }
    }
}

impl ::std::fmt::Debug for self::FieldsInjectedToStruct {
    fn fmt(&self, formatter: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        formatter
            .debug_struct("FieldsInjectedToStruct")
            .field("string_field", &self.string_field)
            .field("injected_field", &self.injected_field)
            .finish()
    }
}

unsafe impl ::std::marker::Send for self::FieldsInjectedToStruct {}
unsafe impl ::std::marker::Sync for self::FieldsInjectedToStruct {}
impl ::std::marker::Unpin for self::FieldsInjectedToStruct {}
impl ::std::panic::RefUnwindSafe for self::FieldsInjectedToStruct {}
impl ::std::panic::UnwindSafe for self::FieldsInjectedToStruct {}

impl ::fbthrift::GetTType for self::FieldsInjectedToStruct {
    const TTYPE: ::fbthrift::TType = ::fbthrift::TType::Struct;
}

impl ::fbthrift::GetTypeNameType for self::FieldsInjectedToStruct {
    fn type_name_type() -> fbthrift::TypeNameType {
        ::fbthrift::TypeNameType::StructType
    }
}

impl<P> ::fbthrift::Serialize<P> for self::FieldsInjectedToStruct
where
    P: ::fbthrift::ProtocolWriter,
{
    #[inline]
    fn write(&self, p: &mut P) {
        p.write_struct_begin("FieldsInjectedToStruct");
        p.write_field_begin("string_field", ::fbthrift::TType::String, 1);
        ::fbthrift::Serialize::write(&self.string_field, p);
        p.write_field_end();
        p.write_field_begin("injected_field", ::fbthrift::TType::String, -1100);
        ::fbthrift::Serialize::write(&self.injected_field, p);
        p.write_field_end();
        p.write_field_stop();
        p.write_struct_end();
    }
}

impl<P> ::fbthrift::Deserialize<P> for self::FieldsInjectedToStruct
where
    P: ::fbthrift::ProtocolReader,
{
    #[inline]
    fn read(p: &mut P) -> ::anyhow::Result<Self> {
        static FIELDS: &[::fbthrift::Field] = &[
            ::fbthrift::Field::new("injected_field", ::fbthrift::TType::String, -1100),
            ::fbthrift::Field::new("string_field", ::fbthrift::TType::String, 1),
        ];
        let mut field_string_field = ::std::option::Option::None;
        let mut field_injected_field = ::std::option::Option::None;
        let _ = ::anyhow::Context::context(p.read_struct_begin(|_| ()), "Expected a FieldsInjectedToStruct")?;
        loop {
            #![allow(unused_imports)]
            use ::anyhow::Context;
            let (_, fty, fid) = p.read_field_begin(|_| (), FIELDS)?;
            match (fty, fid as ::std::primitive::i32) {
                (::fbthrift::TType::Stop, _) => break,
                (::fbthrift::TType::String, 1) => field_string_field = ::std::option::Option::Some(::fbthrift::Deserialize::read(p).with_context(||format!("Error while deserialising string_field field of FieldsInjectedToStruct"))?),
                (::fbthrift::TType::String, -1100) => field_injected_field = ::std::option::Option::Some(::fbthrift::Deserialize::read(p).with_context(||format!("Error while deserialising injected_field field of FieldsInjectedToStruct"))?),
                (fty, _) => p.skip(fty)?,
            }
            p.read_field_end()?;
        }
        p.read_struct_end()?;
        ::std::result::Result::Ok(Self {
            string_field: field_string_field.unwrap_or_default(),
            injected_field: field_injected_field.unwrap_or_default(),
            _dot_dot_Default_default: self::dot_dot::OtherFields(()),
        })
    }
}


impl ::fbthrift::metadata::ThriftAnnotations for FieldsInjectedToStruct {
    fn get_structured_annotation<T: Sized + 'static>() -> ::std::option::Option<T> {
        #[allow(unused_variables)]
        let type_id = ::std::any::TypeId::of::<T>();

        if type_id == ::std::any::TypeId::of::<internal__types::InjectMetadataFields>() {
            let mut tmp = ::std::option::Option::Some(internal__types::InjectMetadataFields {
                r#type: "Fields".to_owned(),
                ..::std::default::Default::default()
            });
            let r: &mut dyn ::std::any::Any = &mut tmp;
            let r: &mut ::std::option::Option<T> = r.downcast_mut().unwrap();
            return r.take();
        }

        ::std::option::Option::None
    }

    fn get_field_structured_annotation<T: Sized + 'static>(field_id: ::std::primitive::i16) -> ::std::option::Option<T> {
        #[allow(unused_variables)]
        let type_id = ::std::any::TypeId::of::<T>();

        #[allow(clippy::match_single_binding)]
        match field_id {
            1 => {
            },
            -1100 => {
            },
            _ => {}
        }

        ::std::option::Option::None
    }
}


#[allow(clippy::derivable_impls)]
impl ::std::default::Default for self::FieldsInjectedWithIncludedStruct {
    fn default() -> Self {
        Self {
            string_field: ::std::default::Default::default(),
            injected_field: ::std::default::Default::default(),
            injected_structured_annotation_field: ::std::option::Option::None,
            injected_unstructured_annotation_field: ::std::option::Option::None,
            _dot_dot_Default_default: self::dot_dot::OtherFields(()),
        }
    }
}

impl ::std::fmt::Debug for self::FieldsInjectedWithIncludedStruct {
    fn fmt(&self, formatter: &mut ::std::fmt::Formatter) -> ::std::fmt::Result {
        formatter
            .debug_struct("FieldsInjectedWithIncludedStruct")
            .field("string_field", &self.string_field)
            .field("injected_field", &self.injected_field)
            .field("injected_structured_annotation_field", &self.injected_structured_annotation_field)
            .field("injected_unstructured_annotation_field", &self.injected_unstructured_annotation_field)
            .finish()
    }
}

unsafe impl ::std::marker::Send for self::FieldsInjectedWithIncludedStruct {}
unsafe impl ::std::marker::Sync for self::FieldsInjectedWithIncludedStruct {}
impl ::std::marker::Unpin for self::FieldsInjectedWithIncludedStruct {}
impl ::std::panic::RefUnwindSafe for self::FieldsInjectedWithIncludedStruct {}
impl ::std::panic::UnwindSafe for self::FieldsInjectedWithIncludedStruct {}

impl ::fbthrift::GetTType for self::FieldsInjectedWithIncludedStruct {
    const TTYPE: ::fbthrift::TType = ::fbthrift::TType::Struct;
}

impl ::fbthrift::GetTypeNameType for self::FieldsInjectedWithIncludedStruct {
    fn type_name_type() -> fbthrift::TypeNameType {
        ::fbthrift::TypeNameType::StructType
    }
}

impl<P> ::fbthrift::Serialize<P> for self::FieldsInjectedWithIncludedStruct
where
    P: ::fbthrift::ProtocolWriter,
{
    #[inline]
    fn write(&self, p: &mut P) {
        p.write_struct_begin("FieldsInjectedWithIncludedStruct");
        p.write_field_begin("string_field", ::fbthrift::TType::String, 1);
        ::fbthrift::Serialize::write(&self.string_field, p);
        p.write_field_end();
        p.write_field_begin("injected_field", ::fbthrift::TType::String, -1100);
        ::fbthrift::Serialize::write(&self.injected_field, p);
        p.write_field_end();
        if let ::std::option::Option::Some(some) = &self.injected_structured_annotation_field {
            p.write_field_begin("injected_structured_annotation_field", ::fbthrift::TType::String, -1101);
            ::fbthrift::Serialize::write(some, p);
            p.write_field_end();
        }
        if let ::std::option::Option::Some(some) = &self.injected_unstructured_annotation_field {
            p.write_field_begin("injected_unstructured_annotation_field", ::fbthrift::TType::String, -1102);
            ::fbthrift::Serialize::write(some, p);
            p.write_field_end();
        }
        p.write_field_stop();
        p.write_struct_end();
    }
}

impl<P> ::fbthrift::Deserialize<P> for self::FieldsInjectedWithIncludedStruct
where
    P: ::fbthrift::ProtocolReader,
{
    #[inline]
    fn read(p: &mut P) -> ::anyhow::Result<Self> {
        static FIELDS: &[::fbthrift::Field] = &[
            ::fbthrift::Field::new("injected_field", ::fbthrift::TType::String, -1100),
            ::fbthrift::Field::new("injected_structured_annotation_field", ::fbthrift::TType::String, -1101),
            ::fbthrift::Field::new("injected_unstructured_annotation_field", ::fbthrift::TType::String, -1102),
            ::fbthrift::Field::new("string_field", ::fbthrift::TType::String, 1),
        ];
        let mut field_string_field = ::std::option::Option::None;
        let mut field_injected_field = ::std::option::Option::None;
        let mut field_injected_structured_annotation_field = ::std::option::Option::None;
        let mut field_injected_unstructured_annotation_field = ::std::option::Option::None;
        let _ = ::anyhow::Context::context(p.read_struct_begin(|_| ()), "Expected a FieldsInjectedWithIncludedStruct")?;
        loop {
            #![allow(unused_imports)]
            use ::anyhow::Context;
            let (_, fty, fid) = p.read_field_begin(|_| (), FIELDS)?;
            match (fty, fid as ::std::primitive::i32) {
                (::fbthrift::TType::Stop, _) => break,
                (::fbthrift::TType::String, 1) => field_string_field = ::std::option::Option::Some(::fbthrift::Deserialize::read(p).with_context(||format!("Error while deserialising string_field field of FieldsInjectedWithIncludedStruct"))?),
                (::fbthrift::TType::String, -1100) => field_injected_field = ::std::option::Option::Some(::fbthrift::Deserialize::read(p).with_context(||format!("Error while deserialising injected_field field of FieldsInjectedWithIncludedStruct"))?),
                (::fbthrift::TType::String, -1101) => field_injected_structured_annotation_field = ::std::option::Option::Some(::fbthrift::Deserialize::read(p).with_context(||format!("Error while deserialising injected_structured_annotation_field field of FieldsInjectedWithIncludedStruct"))?),
                (::fbthrift::TType::String, -1102) => field_injected_unstructured_annotation_field = ::std::option::Option::Some(::fbthrift::Deserialize::read(p).with_context(||format!("Error while deserialising injected_unstructured_annotation_field field of FieldsInjectedWithIncludedStruct"))?),
                (fty, _) => p.skip(fty)?,
            }
            p.read_field_end()?;
        }
        p.read_struct_end()?;
        ::std::result::Result::Ok(Self {
            string_field: field_string_field.unwrap_or_default(),
            injected_field: field_injected_field.unwrap_or_default(),
            injected_structured_annotation_field: field_injected_structured_annotation_field,
            injected_unstructured_annotation_field: field_injected_unstructured_annotation_field,
            _dot_dot_Default_default: self::dot_dot::OtherFields(()),
        })
    }
}


impl ::fbthrift::metadata::ThriftAnnotations for FieldsInjectedWithIncludedStruct {
    fn get_structured_annotation<T: Sized + 'static>() -> ::std::option::Option<T> {
        #[allow(unused_variables)]
        let type_id = ::std::any::TypeId::of::<T>();

        if type_id == ::std::any::TypeId::of::<internal__types::InjectMetadataFields>() {
            let mut tmp = ::std::option::Option::Some(internal__types::InjectMetadataFields {
                r#type: "foo.Fields".to_owned(),
                ..::std::default::Default::default()
            });
            let r: &mut dyn ::std::any::Any = &mut tmp;
            let r: &mut ::std::option::Option<T> = r.downcast_mut().unwrap();
            return r.take();
        }

        ::std::option::Option::None
    }

    fn get_field_structured_annotation<T: Sized + 'static>(field_id: ::std::primitive::i16) -> ::std::option::Option<T> {
        #[allow(unused_variables)]
        let type_id = ::std::any::TypeId::of::<T>();

        #[allow(clippy::match_single_binding)]
        match field_id {
            1 => {
            },
            -1100 => {
            },
            -1101 => {

                if type_id == ::std::any::TypeId::of::<thrift__types::Box>() {
                    let mut tmp = ::std::option::Option::Some(thrift__types::Box {
                        ..::std::default::Default::default()
                    });
                    let r: &mut dyn ::std::any::Any = &mut tmp;
                    let r: &mut ::std::option::Option<T> = r.downcast_mut().unwrap();
                    return r.take();
                }
            },
            -1102 => {

                if type_id == ::std::any::TypeId::of::<thrift__types::Box>() {
                    let mut tmp = ::std::option::Option::Some(thrift__types::Box {
                        ..::std::default::Default::default()
                    });
                    let r: &mut dyn ::std::any::Any = &mut tmp;
                    let r: &mut ::std::option::Option<T> = r.downcast_mut().unwrap();
                    return r.take();
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
