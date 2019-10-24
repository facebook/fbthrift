/*
 * Copyright 2019-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

use crate::protocol::ProtocolWriter;
use crate::ttype::GetTType;
use bytes::Bytes;
use std::collections::{BTreeMap, BTreeSet, HashMap, HashSet};
use std::hash::Hash;

// Write trait. Every type that needs to be serialized will implement this trait.
pub trait Serialize<P>
where
    P: ProtocolWriter,
{
    fn write(self, p: &mut P);
}

impl<P, T> Serialize<P> for Box<T>
where
    P: ProtocolWriter,
    for<'t> &'t T: Serialize<P>,
{
    #[inline]
    fn write(self, p: &mut P) {
        self.as_ref().write(p)
    }
}

impl<'a, P, T> Serialize<P> for &'a Box<T>
where
    P: ProtocolWriter,
    for<'t> &'t T: Serialize<P>,
{
    #[inline]
    fn write(self, p: &mut P) {
        self.as_ref().write(p)
    }
}

impl<P> Serialize<P> for ()
where
    P: ProtocolWriter,
{
    #[inline]
    fn write(self, _p: &mut P) {}
}

impl<'a, P> Serialize<P> for &'a ()
where
    P: ProtocolWriter,
{
    #[inline]
    fn write(self, _p: &mut P) {}
}

impl<P> Serialize<P> for bool
where
    P: ProtocolWriter,
{
    #[inline]
    fn write(self, p: &mut P) {
        p.write_bool(self)
    }
}

impl<'a, P> Serialize<P> for &'a bool
where
    P: ProtocolWriter,
{
    #[inline]
    fn write(self, p: &mut P) {
        p.write_bool(*self)
    }
}

impl<P> Serialize<P> for i8
where
    P: ProtocolWriter,
{
    #[inline]
    fn write(self, p: &mut P) {
        p.write_byte(self)
    }
}

impl<'a, P> Serialize<P> for &'a i8
where
    P: ProtocolWriter,
{
    #[inline]
    fn write(self, p: &mut P) {
        p.write_byte(*self)
    }
}

impl<P> Serialize<P> for i16
where
    P: ProtocolWriter,
{
    #[inline]
    fn write(self, p: &mut P) {
        p.write_i16(self)
    }
}

impl<'a, P> Serialize<P> for &'a i16
where
    P: ProtocolWriter,
{
    #[inline]
    fn write(self, p: &mut P) {
        p.write_i16(*self)
    }
}

impl<P> Serialize<P> for i32
where
    P: ProtocolWriter,
{
    #[inline]
    fn write(self, p: &mut P) {
        p.write_i32(self)
    }
}

impl<'a, P> Serialize<P> for &'a i32
where
    P: ProtocolWriter,
{
    #[inline]
    fn write(self, p: &mut P) {
        p.write_i32(*self)
    }
}

impl<P> Serialize<P> for i64
where
    P: ProtocolWriter,
{
    #[inline]
    fn write(self, p: &mut P) {
        p.write_i64(self)
    }
}

impl<'a, P> Serialize<P> for &'a i64
where
    P: ProtocolWriter,
{
    #[inline]
    fn write(self, p: &mut P) {
        p.write_i64(*self)
    }
}

impl<P> Serialize<P> for f64
where
    P: ProtocolWriter,
{
    #[inline]
    fn write(self, p: &mut P) {
        p.write_double(self)
    }
}

impl<'a, P> Serialize<P> for &'a f64
where
    P: ProtocolWriter,
{
    #[inline]
    fn write(self, p: &mut P) {
        p.write_double(*self)
    }
}

impl<P> Serialize<P> for f32
where
    P: ProtocolWriter,
{
    #[inline]
    fn write(self, p: &mut P) {
        p.write_float(self)
    }
}

impl<'a, P> Serialize<P> for &'a f32
where
    P: ProtocolWriter,
{
    #[inline]
    fn write(self, p: &mut P) {
        p.write_float(*self)
    }
}

impl<'a, P> Serialize<P> for &'a String
where
    P: ProtocolWriter,
{
    #[inline]
    fn write(self, p: &mut P) {
        p.write_string(self.as_str())
    }
}

impl<'a, P> Serialize<P> for &'a str
where
    P: ProtocolWriter,
{
    #[inline]
    fn write(self, p: &mut P) {
        p.write_string(self)
    }
}

impl<'a, P> Serialize<P> for &'a Bytes
where
    P: ProtocolWriter,
{
    #[inline]
    fn write(self, p: &mut P) {
        p.write_binary(self.as_ref())
    }
}

impl<'a, P> Serialize<P> for &'a Vec<u8>
where
    P: ProtocolWriter,
{
    #[inline]
    fn write(self, p: &mut P) {
        p.write_binary(self.as_ref())
    }
}

impl<'a, P> Serialize<P> for &'a [u8]
where
    P: ProtocolWriter,
{
    #[inline]
    fn write(self, p: &mut P) {
        p.write_binary(self)
    }
}

impl<'a, P, T> Serialize<P> for &'a BTreeSet<T>
where
    P: ProtocolWriter,
    T: GetTType + Ord,
    &'a T: Serialize<P>,
{
    fn write(self, p: &mut P) {
        p.write_set_begin(T::TTYPE, self.len());
        for item in self.iter() {
            item.write(p);
        }
        p.write_set_end();
    }
}

impl<'a, P, T> Serialize<P> for &'a HashSet<T>
where
    P: ProtocolWriter,
    T: GetTType + Hash + Eq,
    &'a T: Serialize<P>,
{
    fn write(self, p: &mut P) {
        p.write_set_begin(T::TTYPE, self.len());
        for item in self.iter() {
            item.write(p);
        }
        p.write_set_end();
    }
}

impl<'a, P, K, V> Serialize<P> for &'a BTreeMap<K, V>
where
    P: ProtocolWriter,
    K: GetTType + Ord,
    &'a K: Serialize<P>,
    V: GetTType,
    &'a V: Serialize<P>,
{
    fn write(self, p: &mut P) {
        p.write_map_begin(K::TTYPE, V::TTYPE, self.len());
        for (k, v) in self.iter() {
            k.write(p);
            v.write(p);
        }
        p.write_map_end();
    }
}

impl<'a, P, K, V> Serialize<P> for &'a HashMap<K, V>
where
    P: ProtocolWriter,
    K: GetTType + Hash + Eq,
    &'a K: Serialize<P>,
    V: GetTType,
    &'a V: Serialize<P>,
{
    fn write(self, p: &mut P) {
        p.write_map_begin(K::TTYPE, V::TTYPE, self.len());
        for (k, v) in self.iter() {
            k.write(p);
            v.write(p);
        }
        p.write_map_end();
    }
}

impl<'a, P, T> Serialize<P> for &'a Vec<T>
where
    P: ProtocolWriter,
    T: GetTType,
    &'a T: Serialize<P>,
{
    /// Vec<T> is Thrift List type
    fn write(self, p: &mut P) {
        p.write_list_begin(T::TTYPE, self.len());
        for item in self.iter() {
            item.write(p);
        }
        p.write_list_end();
    }
}

impl<'a, P, T> Serialize<P> for &'a [T]
where
    P: ProtocolWriter,
    T: GetTType,
    &'a T: Serialize<P>,
{
    /// \[T\] is Thrift List type
    fn write(self, p: &mut P) {
        p.write_list_begin(T::TTYPE, self.len());
        for item in self.iter() {
            item.write(p);
        }
        p.write_list_end();
    }
}
