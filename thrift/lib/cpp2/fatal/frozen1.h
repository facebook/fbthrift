/*
 * Copyright 2016-present Facebook, Inc.
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
#ifndef THRIFT_FATAL_FROZEN1_H_
#define THRIFT_FATAL_FROZEN1_H_ 1

#include <thrift/lib/cpp/Frozen.h>
#include <thrift/lib/cpp2/fatal/reflection.h>

#include <fatal/type/accumulate.h>
#include <fatal/type/conditional.h>
#include <fatal/type/list.h>
#include <fatal/type/push.h>
#include <fatal/type/sequence.h>
#include <fatal/type/sort.h>

#include <cstdint>

namespace apache {
namespace thrift {
namespace frzn_dtl {

///////////////////
// READ ME FIRST //
//////////////////////////////////////////////////////////////////////////////
// Look for `START HERE` for the `Frozen` specialization for structs, which //
// triggers all the layout calculation and artificial layout creation.      //
//////////////////////////////////////////////////////////////////////////////

/**
 * layout_raw: virtual layout for arbitrary types
 */

template <std::ptrdiff_t ActualOffset, std::ptrdiff_t DesiredOffset, typename T>
struct l {
  inline T const* operator->() const {
    return reinterpret_cast<T const*>(
        reinterpret_cast<std::uint8_t const*>(this) +
        (DesiredOffset - ActualOffset));
  }

  inline T const& operator*() const {
    return *reinterpret_cast<T const*>(
        reinterpret_cast<std::uint8_t const*>(this) +
        (DesiredOffset - ActualOffset));
  }

  /* implicit */ inline operator T const&() const {
    return **this;
  }
};

template <std::ptrdiff_t ActualOffset, std::ptrdiff_t DesiredOffset, typename T>
using layout_raw = l<ActualOffset, DesiredOffset, T>;

/**
 * layout_bit: virtual layout for packed bits
 */

template <
    std::ptrdiff_t ActualOffset,
    std::ptrdiff_t DesiredOffset,
    std::size_t Bit>
struct L {
  static_assert(Bit < CHAR_BIT, "internal error");

  /* implicit */ inline operator bool() const { // @nolint
    return static_cast<bool>(
        reinterpret_cast<std::uint8_t const*>(
            this)[DesiredOffset - ActualOffset] &
        (1u << Bit));
  }

  inline bool operator*() const {
    return static_cast<bool>(*this);
  }
};

template <
    std::ptrdiff_t ActualOffset,
    std::ptrdiff_t DesiredOffset,
    std::size_t Bit>
using layout_bit = L<ActualOffset, DesiredOffset, Bit>;

/**
 * align_up: a helper to calculate alignment of frozen1 fields
 A
 */
template <typename T>
constexpr inline T align_up(T UnalignedOffset, T Alignment) {
  return UnalignedOffset % Alignment
      ? UnalignedOffset + (Alignment - (UnalignedOffset % Alignment))
      : UnalignedOffset;
}

/**
 * helpers used by `isset_struct`
 */

// creates named members with given type - basically, an adapter that exposes
// `reflected_struct_data_member::pod` as a transform
template <typename T>
struct p {
  template <typename Member>
  using apply = typename Member::template pod<T>;
};

// predicate to filter not required members //
struct F {
  template <typename Member>
  using apply = std::integral_constant<
      bool,
      Member::optional::value != apache::thrift::optionality::required>;
};

/**
 * isset_struct: simulated struct for representing `__isset` for a given
 * struct `T`
 */

// implementation of `isset_struct` //
template <typename T>
struct i : fatal::apply_to<
               fatal::transform<
                   fatal::filter<typename reflect_struct<T>::members, F>,
                   p<bool>>,
               fatal::inherit> {};

template <typename T>
using isset_struct = i<T>;

/**
 * refl_type_class: emulated `reflect_type_class` that automatically recognizes
 * the artificial structure `__isset`
 */

template <typename T>
struct C {
  using type = reflect_type_class<T>;
};

template <typename T>
struct C<isset_struct<T>> {
  using type = type_class::structure;
};

template <typename T>
using refl_type_class = typename C<T>::type;

/**
 * simulated_struct: simulates a struct containing the given members for
 * size/alignment calculation purposes
 */

// a helper that represents a given member - the name of the member is
// unimportant, the only purpose is laying out all members of a struct and
// correctly calculating size/alignment
//
// the integral parameter helps uniquely identify more than one member with the
// same type
template <std::size_t, typename T>
struct D {
  T x;
};

// implementation of simulated struct
template <typename...>
struct S;

template <template <typename...> class V, typename... T, std::size_t... I>
struct S<V<T...>, fatal::index_sequence<I...>> : D<I, T>... {};

template <typename MemberTypes>
using simulated_struct =
    S<MemberTypes, fatal::make_index_sequence<fatal::size<MemberTypes>::value>>;

/**
 * physical_layout: calculates the physical size of an artificial frozen1 layout
 */

// implementation of `physical_layout` - specialization for structures can be
// found further down this file due do cyclic dependencies
template <typename T, typename TypeClass>
struct P {
  using type = typename apache::thrift::Freezer<T>::FrozenType;
};

template <typename T>
using physical_layout = typename P<T, refl_type_class<T>>::type;

/**
 * calculate_layout: calculates the correct frozen1 offsets for each member of a
 * given struct `T`. Uses `fatal::accumulate` on the members list, passing an
 * accumulator that keeps track of the offsets of the members, adjusting for
 * size, alignment and padding.
 */

// frozen1 layout calculation accumulator for use with fatal::accumulate //
template <
    std::ptrdiff_t ActualOffset = 0,
    std::ptrdiff_t DesiredOffset = 0,
    std::size_t BitIndex = 0,
    typename PhysicalLayout = fatal::list<>,
    typename... FrozenLayout>
struct A {
  using frozen = fatal::list<FrozenLayout...>;
  using physical = PhysicalLayout;

  template <
      typename Member,
      typename PhysicalType,
      bool IsBool,
      std::ptrdiff_t AlignedOffset>
  using impl =
      A<ActualOffset + 1,
        AlignedOffset +
            (IsBool ? BitIndex + 1 == CHAR_BIT : sizeof(PhysicalType)),
        IsBool ? (BitIndex + 1) % CHAR_BIT : 0,
        fatal::push_back_if<!IsBool || !BitIndex, PhysicalLayout, PhysicalType>,
        FrozenLayout...,
        typename Member::template pod<fatal::conditional<
            std::is_same<bool, typename Member::type>::value,
            layout_bit<ActualOffset, AlignedOffset, BitIndex>,
            layout_raw<
                ActualOffset,
                AlignedOffset,
                typename apache::thrift::Freezer<
                    typename Member::type>::FrozenType>>>>;

  template <typename Member>
  using apply = impl<
      Member,
      physical_layout<typename Member::type>,
      std::is_same<bool, typename Member::type>::value,
      std::is_same<bool, typename Member::type>::value
          ? DesiredOffset
          : align_up<std::ptrdiff_t>(
                DesiredOffset + (BitIndex != 0),
                alignof(physical_layout<typename Member::type>))>;
};

// emulated `reflected_struct_data_member` for the `__isset` member //
template <typename T>
struct N {
  using type = isset_struct<T>;
  using type_class = type_class::structure;
  using optional = std::integral_constant<optionality, optionality::required>;

  template <typename U>
  struct pod {
    U __isset;
  };
};

// emulated `reflected_struct_data_member` for `__isset` members //
template <typename Member>
struct n {
  using type = bool;
  using type_class = type_class::integral;
  using optional = std::integral_constant<optionality, optionality::required>;
  template <typename T>
  using pod = typename Member::template pod<T>;
};

// emulated `reflect_struct` that automatically adds an artificial `__isset`
// member to any given struct
template <typename T>
struct r {
  using type = T;
  using members = fatal::push_back_if<
      !fatal::empty<typename r<isset_struct<T>>::members>::value,
      typename reflect_struct<T>::members,
      N<T>>;
};

// specialization of the emulated `reflected_struct` for the artificial
// `__isset` structure - needed to avoid infinite recursion when adding the
// `__isset` member to structures
template <typename T>
struct r<isset_struct<T>> {
  using type = isset_struct<T>;

  using members = fatal::transform<
      fatal::filter<typename reflect_struct<T>::members, F>,
      fatal::applier<n>>;
};

template <typename T>
using calculate_layout = fatal::accumulate<typename r<T>::members, A<>>;

/**
 * frozen1_layout: computes a structure that represents the frozen1 layout of
 * given structure `T`
 */

template <typename T>
using frozen1_layout =
    fatal::apply_to<typename calculate_layout<T>::frozen, fatal::inherit>;

/**
 * enable_if_struct: helper to enable_if when T is a structure
 */

template <typename T>
using enable_if_struct = typename std::enable_if<
    std::is_same<type_class::structure, refl_type_class<T>>::value>::type;

/**
 * FrozenIterator: const iterator for artificial frozen1 layouts
 */

template <typename FrozenType>
struct I {
  using iterator_category = std::random_access_iterator_tag;
  using value_type = FrozenType const;
  using pointer = value_type*;
  using reference = value_type&;
  using difference_type = std::ptrdiff_t;

  explicit I(pointer i) : i_(i) {}

  I operator++(int) const {
    auto copy(*this);
    ++*this;
    return copy;
  }

  I operator--(int) const {
    auto copy(*this);
    --*this;
    return copy;
  }

  I& operator++() {
    i_ = reinterpret_cast<pointer>(
        reinterpret_cast<char const*>(i_) + FrozenSizeOf<FrozenType>::size());
    return *this;
  }

  I& operator--() {
    i_ = reinterpret_cast<pointer>(
        reinterpret_cast<char const*>(i_) - FrozenSizeOf<FrozenType>::size());
    return *this;
  }

  I operator+(difference_type increment) const {
    DCHECK_EQ(0, increment % FrozenSizeOf<FrozenType>::size());
    return I(reinterpret_cast<pointer>(
        reinterpret_cast<char const*>(i_) +
        increment * FrozenSizeOf<FrozenType>::size()));
  }

  I operator-(difference_type decrement) const {
    DCHECK_EQ(0, decrement % FrozenSizeOf<FrozenType>::size());
    return I(reinterpret_cast<pointer>(
        reinterpret_cast<char const*>(i_) -
        decrement * FrozenSizeOf<FrozenType>::size()));
  }

  I& operator+=(difference_type increment) {
    return *this = *this + increment;
  }

  I& operator-=(difference_type increment) {
    return *this = *this - increment;
  }

  difference_type operator-(I const& rhs) const {
    DCHECK_EQ(
        0,
        (reinterpret_cast<char const*>(i_) -
         reinterpret_cast<char const*>(rhs.i_)) %
            FrozenSizeOf<FrozenType>::size());
    return (reinterpret_cast<char const*>(i_) -
            reinterpret_cast<char const*>(rhs.i_)) /
        FrozenSizeOf<FrozenType>::size();
  }

  pointer operator->() const {
    return i_;
  }
  reference operator*() const {
    return *i_;
  }

  bool operator==(I const& rhs) const {
    return i_ == rhs.i_;
  }
  bool operator!=(I const& rhs) const {
    return i_ != rhs.i_;
  }
  bool operator<(I const& rhs) const {
    return i_ < rhs.i_;
  }
  bool operator<=(I const& rhs) const {
    return i_ <= rhs.i_;
  }
  bool operator>(I const& rhs) const {
    return i_ > rhs.i_;
  }
  bool operator>=(I const& rhs) const {
    return i_ >= rhs.i_;
  }

 private:
  pointer i_;
};

/**
 * specializations of classes defined previously
 */

// specialization of `physical_layout` for artificial layouts //
template <typename T>
struct P<T, type_class::structure> {
  using type = simulated_struct<typename calculate_layout<T>::physical>;
};

// specializations of `FrozenSizeOf` for artificial layouts //
template <typename T>
struct z<Frozen<T, enable_if_struct<T>>> {
  static constexpr inline std::size_t size() {
    return sizeof(physical_layout<T>);
  }
  static constexpr inline std::size_t alignment() {
    return alignof(physical_layout<T>);
  }
};

template <typename First, typename Second>
struct z<std::pair<First, Frozen<Second, enable_if_struct<Second>>>> {
  static constexpr inline std::size_t size() {
    return sizeof(std::pair<First, physical_layout<Second>>);
  }
  static constexpr inline std::size_t alignment() {
    return alignof(std::pair<First, physical_layout<Second>>);
  }
};

template <typename First, typename Second>
struct z<std::pair<Frozen<First, enable_if_struct<First>> const, Second>> {
  static constexpr inline std::size_t size() {
    return sizeof(std::pair<physical_layout<First> const, Second>);
  }
  static constexpr inline std::size_t alignment() {
    return alignof(std::pair<physical_layout<First> const, Second>);
  }
};

template <typename First, typename Second>
struct z<std::pair<
    Frozen<First, enable_if_struct<First>> const,
    Frozen<Second, enable_if_struct<Second>>>> {
  static constexpr inline std::size_t size() {
    return sizeof(
        std::pair<physical_layout<First> const, physical_layout<Second>>);
  }
  static constexpr inline std::size_t alignment() {
    return alignof(
        std::pair<physical_layout<First> const, physical_layout<Second>>);
  }
};

// specializations of `FrozenIterator` for artificial layouts //
template <typename FrozenItem>
struct t<Frozen<FrozenItem, enable_if_struct<FrozenItem>>> {
  using type = I<Frozen<FrozenItem>>;
};

template <typename FrozenFirst, typename Second>
struct t<std::pair<
    Frozen<FrozenFirst, enable_if_struct<FrozenFirst>> const,
    Second>> {
  using type = I<std::pair<Frozen<FrozenFirst> const, Second>>;
};

template <typename First, typename FrozenSecond>
struct t<
    std::pair<First, Frozen<FrozenSecond, enable_if_struct<FrozenSecond>>>> {
  using type = I<std::pair<First, Frozen<FrozenSecond>>>;
};

template <typename FrozenFirst, typename FrozenSecond>
struct t<std::pair<
    Frozen<FrozenFirst, enable_if_struct<FrozenFirst>> const,
    Frozen<FrozenSecond, enable_if_struct<FrozenSecond>>>> {
  using type = I<std::pair<Frozen<FrozenFirst> const, Frozen<FrozenSecond>>>;
};

} // namespace frzn_dtl

////////////////
// START HERE //
////////////////

// integration of the cpp2 implementation into the frozen1 library
// this is where it all starts, the rest is just implementation mumbo jumbo
template <typename T>
struct Frozen<T, frzn_dtl::enable_if_struct<T>>
    : public frzn_dtl::frozen1_layout<T> {};

} // namespace thrift
} // namespace apache

#endif // THRIFT_FATAL_FROZEN1_H_
