/*
 * Copyright 2014 Facebook, Inc.
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

#pragma once

#include <iosfwd>
#include <iterator>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <folly/Bits.h>
#include <folly/Demangle.h>
#include <folly/Hash.h>
#include <folly/MapUtil.h>
#include <folly/Optional.h>
#include <folly/Range.h>
#include <folly/experimental/Bits.h>

#include <thrift/lib/cpp2/frozen/FrozenMacros.h>
#include <thrift/lib/cpp2/frozen/Traits.h>
#include <thrift/lib/cpp2/frozen/schema/MemorySchema.h>

#include <thrift/lib/thrift/gen-cpp/frozen_types.h>

namespace apache { namespace thrift { namespace frozen {
/**
 *          \__  __/             \__  __/             \__  __/
 *          /_/  \_\             /_/  \_\             /_/  \_\
 *           _\/\/_               _\/\/_               _\/\/_
 *      __/\_\_\/_/_/\__     __/\_\_\/_/_/\__     __/\_\_\/_/_/\__
 *        \/ /_/\_\ \/         \/ /_/\_\ \/         \/ /_/\_\ \/
 *          __/\/\__             __/\/\__             __/\/\__
 *          \_\  /_/             \_\  /_/             \_\  /_/
 *          /      \             /      \             /      \
 *
 * Frozen is a library for storing a serialized representation of a thrift
 * structure with an associated schema, allowing the serialized representation
 * to be used in-place, without deserializing any more data than is needed.
 * These are especially useful for memory-mapped persistent structures.
 *
 * Every value in a frozen structure is stored in a sequence of bits in memory.
 * The layout of the data is computed by recursively visiting all fields of all
 * sub-structures, measuring how much space is needed to store each value.
 *
 * Sizes from one layout to the next may vary based on the distribution of
 * values. In particular, integers are encoded using the smallest binary
 * representation possible for accurately representing the integer values.
 *
 * Variable-length structures such as vectors and strings are stored immediately
 * after the root object, recursively. These areas are referenced with relative
 * addresses, so the entire frozen subtree can freely be relocated in memory.
 *
 * -Tom Jackson
 */
typedef uint8_t byte;

/**
 * Simply represents an indented line separator for use in debugging
 */
struct DebugLine {
  int level;
  explicit DebugLine(int level) : level(level) {}
};

std::ostream& operator<<(std::ostream& os, DebugLine dl);

/**
 * The layout position of a field within a structure.
 */
struct FieldPosition {
  int32_t offset;    // byte offset from owning structure's start
  int32_t bitOffset; // bit offset from owning structure's start
  explicit FieldPosition(int32_t offset = 0, int32_t bitOffset = 0)
      : offset(offset), bitOffset(bitOffset) {
    DCHECK(!offset || !bitOffset);
  }
};

/**
 * The relative position of an an object from the layout root, with bit
 * granularity.
 */
struct LayoutPosition {
  /**
   * The object will be laid out starting 'bitOffset' bits past 'start' bytes
   * past the layout root. 'bitOffset' may exceed 8.
   */
  size_t start;
  size_t bitOffset;

  /**
   * Given a structure starting at *this, return the LayoutPosition of one of
   * its fields.
   */
  LayoutPosition operator()(FieldPosition f) const {
    return {start + f.offset, bitOffset + f.bitOffset};
  }
};

/**
 * Absolute target position in memory for freezing an object, with bit
 * granularity.
 */
struct FreezePosition {
  /**
   * The object will be frozen starting at 'bitOffset' bits past 'start'.
   * 'bitOffset' may exceed 8.
   */
  byte* start;
  size_t bitOffset;

  /**
   * Given a structure starting at *this, return the FreezePosition of one of
   * its fields.
   */
  FreezePosition operator()(FieldPosition f) const {
    return {start + f.offset, bitOffset + f.bitOffset};
  }
};

/**
 * Absolute position in memory for viewing an object, with bit
 * granularity.
 */
struct ViewPosition {
  /**
   * The object to view is located 'bitOffset' bits past 'start'.
   * 'bitOffset' may exceed 8.
   */
  const byte* start;
  size_t bitOffset;
  ViewPosition operator()(FieldPosition f) const {
    return {start + f.offset, bitOffset + f.bitOffset};
  }
};

/**
 * LayoutBase is the common base of all layouts, which will specialize
 * Layout<T>.  Layout<T> represents all information needed to specify the layout
 * of a frozen representation of T in memory. This usually includes the size of
 * the object in bytes (or bits) and sufficient information for accessing any
 * fields, given a ViewPosition and a field.
 */
struct LayoutBase {
  /**
   * The number of bytes occupied by this layout. If this layout was not
   * inlined, this also includes enough bytes to store 'bits' bits. If this is
   * inlined, size will be zero, and any bits for this layout will be allocated
   * in the parent struct's layout.
   */
  size_t size = 0;
  /**
   * Number of bits stored within this object.
   */
  size_t bits = 0;
  /**
   * Indicates that this type's fields are laid out relative to its parent's
   * layout.
   */
  bool inlined = false;
  std::type_index type;

  /**
   * Default constructor: Initializes a fully usable zero-byte layout. A view of
   * such a layout will always produce a default value. This is especially
   * needed for representing fields which were not present in a serialized
   * structure.
   */
  explicit LayoutBase(std::type_index type) : type(std::move(type)) {}

  /**
   * Internal: Updates the size of this structure according the the result of a
   * layout attempt. Returns true iff another pass of layout will be needed.
   */
  bool resize(FieldPosition after, bool inlined);

  /**
   * Convenience function for placing the first field for layout.
   */
  FieldPosition startFieldPosition() const {
    return FieldPosition(inlined ? 0 : (bits + 7) / 8);
  }

  /**
   * Indicates that this layout requires no storage, so saving and freezing may
   * be skipped
   */
  bool empty() const { return !size && !bits; }

  virtual ~LayoutBase() {}

  /**
   * Clears the layout back to a zero-byte layout, recursively.
   */
  virtual void clear();

  /**
   * Prints a description of this layout to the given stream, recursively
   */
  virtual void print(std::ostream& os, int level) const;

  /**
   * Populates a 'layout' with a description of this layout in the context of
   * 'schema'. Child classes must implement.
   */
  template <typename SchemaInfo>
  void save(typename SchemaInfo::Schema& schema,
            typename SchemaInfo::Layout& layout,
            typename SchemaInfo::Helper& helper) const {
    layout.setSize(size);
    layout.setBits(bits);
  }

  /**
   * Populates this layout from the description stored in 'layout' in the
   * context of 'schema'. Child classes must implement.
   */
  template <typename SchemaInfo>
  void load(const typename SchemaInfo::Schema& schema,
            const typename SchemaInfo::Layout& layout) {
    size = layout.getSize();
    bits = layout.getBits();
  }

 protected:
  LayoutBase(const LayoutBase&) = default;
  LayoutBase(LayoutBase&&) = default;
};

template <class T, class = void>
struct Layout : public LayoutBase {
  static_assert(sizeof(T) == 0, "Objects of this type cannot be frozen yet.");
};

template <typename T, typename SchemaInfo = schema::SchemaInfo>
void saveRoot(const Layout<T>& layout, typename SchemaInfo::Schema& schema) {
  typename SchemaInfo::Helper helper(schema);
  typename SchemaInfo::Layout myLayout;
  layout.template save<SchemaInfo>(schema, myLayout, helper);
  schema.setRootLayoutId(std::move(helper.add(std::move(myLayout))));
}

template <typename T, typename SchemaInfo = schema::SchemaInfo>
void loadRoot(Layout<T>& layout, const typename SchemaInfo::Schema& schema) {
  layout.template load<SchemaInfo>(schema, schema.getRootLayout());
}

std::ostream& operator<<(std::ostream& os, const LayoutBase& layout);

/**
 * FieldBase (with concrete implementations provided by Field<T,...>) represents
 * a field within a layout. This includes both its position within the parent
 * struct and the actual layout of the child value.
 *
 * Each field hosts a unique layout which is minimal for storing all values
 * within this field. As an example, a struct with many int fields will have a
 * specialized layout for *each* field, allowing these fields to be sized
 * differently depending on the range of their values.
 *
 * Usually instantiated like:
 *   template<>
 *   class Layout<Person> : LayoutBase {
 *     Field<std::string> name
 *     Layout() : name(1) {}
 *     ...
 *    };
 *
 * Fields require IDs to be specified for versioning.
 */
struct FieldBase {
  /**
   * Thrift field key of this field
   */
  const int32_t key;
  /**
   * Offset of this field within the parent struct
   */
  FieldPosition pos;
  const char* name;

  explicit FieldBase(int32_t key, const char* name) : key(key), name(name) {}
  virtual ~FieldBase() {}

  virtual void clear() = 0;
};

template <class T, class Layout = Layout<typename std::decay<T>::type>>
struct Field final : public FieldBase {
  Layout layout;

  explicit Field(int32_t key, const char* name) : FieldBase(key, name) {}

  /**
   * Prints a description of this layout to the given stream, recursively.
   */
  void print(std::ostream& os, int level) const {
    os << DebugLine(level) << name;
    if (pos.offset) {
      os << " @ offset " << pos.offset;
    } else if (pos.bitOffset) {
      os << " @ bit " << pos.bitOffset;
    } else {
      os << " @ start";
    }
    layout.print(os, level + 1);
  }

  /**
   * Clears this subtree's layout, changing the layout to 0 bytes.
   */
  void clear() override { layout.clear(); }

  /**
   * Populates the layout information for this field from the description of
   * this field in the parent layout, identified by key.
   */
  template <typename SchemaInfo>
  void load(const typename SchemaInfo::Schema& schema,
            const typename SchemaInfo::Field& field) {
    auto offset = field.getOffset();
    if (offset < 0) {
      pos.bitOffset = -offset;
    } else {
      pos.offset = offset;
    }
    this->layout.template load<SchemaInfo>(schema,
                                           schema.getLayoutForField(field));
  }

  /**
   * Recursively stores the layout information for this field, including both
   * field offset information and the information for the contained layout.
   */
  template <typename SchemaInfo>
  void save(typename SchemaInfo::Schema& schema,
            typename SchemaInfo::Layout& parent,
            typename SchemaInfo::Helper& helper) const {
    if (this->layout.empty()) {
      return;
    }

    typename SchemaInfo::Field field;
    field.setId(key);
    if (pos.bitOffset) {
      field.setOffset(-pos.bitOffset);
    } else {
      field.setOffset(pos.offset);
    }

    typename SchemaInfo::Layout myLayout;
    this->layout.template save<SchemaInfo>(schema, myLayout, helper);
    field.setLayoutId(std::move(helper.add(std::move(myLayout))));
    parent.addField(std::move(field));
  }
};

/**
 * Views (which all inherit from ViewBase) represent a view of a frozen object
 * for use. Note that the storage of the layout used by a view must be managed
 * separately.
 */
template <class Self, class Layout, class T>
class ViewBase {
 protected:
  /**
   * Unowned pointer to the layout of this object.
   */
  const Layout* layout_;
  /**
   * Position in memory to view.
   */
  ViewPosition position_;

  static const Layout* defaultLayout() {
    static Layout layout;
    return &layout;
  }

 public:
  ViewBase() : layout_(defaultLayout()), position_({nullptr, 0}) {}

  ViewBase(const Layout* layout, ViewPosition position)
      : layout_(layout), position_(position) {}


  explicit operator bool() const {
    return position_.start && !layout_->empty();
  }

  /**
   * thaw this object back into its original, mutable representation.
   */
  T thaw() const {
    T ret;
    layout_->thaw(position_, ret);
    return ret;
  }

  const Self* operator->() const { return static_cast<const Self*>(this); }
};

/*
 * thaw() either thaws a view or passes through the input if the value is an
 * eagerly thawed type.
 */
template <class Self, class Layout, class T>
T thaw(const ViewBase<Self, Layout, T>& view) {
  return view.thaw();
};

template <class T>
T thaw(T value) {
  return value;
};

/**
 * LayoutRoot calculates the layout necessary to store a given object,
 * recursively. The logic of layout should closely match that of freezing.
 */
class LayoutRoot {
  LayoutRoot() {}
  /**
   * Lays out a given object from the root, repeatedly running layout until a
   * fixed point is reached.
   */
  template <class T>
  size_t doLayout(const T& root, Layout<T>& layout) {
    for (int t = 0; t < 1000; ++t) {
      resized_ = false;
      cursor_ = layout.size;
      auto after = layout.layout(*this, root, {0, 0});
      if (!layout.resize(after, false) && !resized_) {
        return cursor_;
      }
    }
    assert(false); // layout should always reach a fixed point.
    return 0;
  }

 public:
  /**
   * Adjust 'layout' so it is sufficient for freezing root, and return the total
   * number of bytes needed to store this object.
   */
  template <class T>
  static size_t layout(const T& root, Layout<T>& layout) {
    return LayoutRoot().doLayout(root, layout);
  }

  /**
   * Internal utility for recursing into child fields.
   *
   * Lays out 'field' at position 'fieldPos', then recurse into the field value
   * to adjust 'field.layout'.
   */
  template <class T, class Layout, class Arg>
  FieldPosition layoutField(LayoutPosition self,
                            FieldPosition fieldPos,
                            Field<T, Layout>& field,
                            const Arg& value) {
    auto& layout = field.layout;
    bool inlineBits = layout.size == 0;
    FieldPosition nextPos = fieldPos;
    if (inlineBits) {
      //  candidate for inlining, place at offset zero and continue from 'self'
      FieldPosition inlinedField(0, fieldPos.bitOffset);
      FieldPosition after = layout.layout(*this, value, self(inlinedField));
      if (after.offset) {
        // consumed full bytes for layout, can't be inlined
        inlineBits = false;
      } else {
        // only consumed bits, layout at bit offset
        resized_ = layout.resize(after, true) || resized_;
        field.pos = inlinedField;
        nextPos.bitOffset += layout.bits;
      }
    }
    if (!inlineBits) {
      FieldPosition normalField(fieldPos.offset, 0);
      FieldPosition after = layout.layout(*this, value, self(normalField));
      resized_ = layout.resize(after, false) || resized_;
      field.pos = normalField;
      nextPos.offset += layout.size;
    }
    return nextPos;
  }

  template <class T, class Layout>
  FieldPosition layoutOptionalField(LayoutPosition self,
                                    FieldPosition fieldPos,
                                    Field<folly::Optional<T>, Layout>& field,
                                    bool present,
                                    const T& value) {
    if (present) {
      return layoutField(self, fieldPos, field, value);
    } else {
      return layoutField(self, fieldPos, field, folly::none);
    }
  }

  /**
   * Simulates appending count bytes, returning their offset (in bytes) from
   * origin.
   */
  size_t layoutBytesDistance(size_t origin, size_t count) {
    if (count == 0) {
      return 0;
    }
    if (cursor_ < origin) {
      cursor_ = origin;
    }
    size_t start = cursor_;
    cursor_ += count;
    return start - origin;
  }

 protected:
  bool resized_;
  size_t cursor_;
};

/**
 * LayoutException is thrown if freezing is attempted without a sufficient
 * layout
 */
class LayoutException : public std::length_error {
 public:
  LayoutException()
      : std::length_error("Existing layouts insufficient for this object") {}
};

/**
 * LayoutTypeMismatch is thrown if the type of a field is incompatible with the
 * type specified in a schema. This may be relaxed by setting
 * 'schema.relaxTypeChecks'.
 */
class LayoutTypeMismatchException : public std::logic_error {
 public:
  LayoutTypeMismatchException(const std::string& expected,
                              const std::string& actual)
      : std::logic_error("Layout for '" + expected +
                         "' loaded from layout of '" + actual + "'") {}
};

/**
 * FreezeRoot freezes a root object according to the given layout. Storage
 * management is defined by a child class of FreezeRoot.
 */
class FreezeRoot {
 protected:
  template <class T>
  typename Layout<T>::View doFreeze(const Layout<T>& layout, const T& root) {
    folly::MutableByteRange range;
    size_t dist;
    appendBytes(0, layout.size, range, dist);
    layout.freeze(*this, root, {range.begin(), 0});
    return layout.view({range.begin(), 0});
  }

 public:
  virtual ~FreezeRoot() {}

  /**
   * Internal utility for recursing into child fields.
   *
   * Freezes 'value' into a 'field' of an object located at 'self'.
   */
  template <class T, class Layout, class Arg>
  void freezeField(FreezePosition self,
                   const Field<T, Layout>& field,
                   const Arg& value) {
    if (!field.layout.empty()) {
      field.layout.freeze(*this, value, self(field.pos));
    }
  }

  template <class T, class Layout>
  void freezeOptionalField(FreezePosition self,
                           const Field<folly::Optional<T>, Layout>& field,
                           bool present,
                           const T& value) {
    if (present) {
      freezeField(self, field, value);
    } else {
      freezeField(self, field, folly::none);
    }
  }

  /**
   * Appends bytes to the store, setting an output range and a distance from a
   * given origin.
   */
  void appendBytes(byte* origin,
                   size_t n,
                   folly::MutableByteRange& range,
                   size_t& distance) {
    doAppendBytes(origin, n, range, distance);
  }

 private:
  virtual void doAppendBytes(byte* origin,
                             size_t n,
                             folly::MutableByteRange& range,
                             size_t& distance) = 0;
};

/**
 * A FreezeRoot that writes to a given ByteRange
 */
class ByteRangeFreezer final : public FreezeRoot {
 protected:
  explicit ByteRangeFreezer(folly::MutableByteRange write) : write_(write) {}

 public:
  template <class T>
  static typename Layout<T>::View freeze(const Layout<T>& layout,
                                         const T& root,
                                         folly::MutableByteRange write) {
    return ByteRangeFreezer(write).doFreeze(layout, root);
  }

 private:
  void doAppendBytes(byte* origin,
                     size_t n,
                     folly::MutableByteRange& range,
                     size_t& distance) override {
    range.reset(write_.begin(), n);
    if (n) {
      if (n > write_.size() || origin > write_.begin()) {
        throw LayoutException();
      }
      distance = write_.begin() - origin;
      write_.advance(n);
    } else {
      distance = 0;
    }
  }

  folly::MutableByteRange write_;
};

struct Holder {
  virtual ~Holder() {};
};

template <class T>
struct HolderImpl : public Holder {
  explicit HolderImpl(const T& t) : t_(t) {}
  explicit HolderImpl(T&& t) : t_(std::move(t)) {}
  T t_;
};

/**
 * Bundled simply subclasses a given class and supports holding objects depended
 * upon by the Base object.
 */
template <class Base>
class Bundled : public Base {
 public:
  explicit Bundled(Base&& base) : Base(std::move(base)) {}
  explicit Bundled(const Base& base) : Base(base) {}

  template <class T>
  void hold(T&& t) {
    holds_.emplace_back(
        new HolderImpl<typename std::decay<T>::type>(std::forward<T>(t)));
  }

 private:
  std::vector<std::unique_ptr<Holder>> holds_;
};

// Enables disambiguated calls to freeze(), which also exists in frozen1
enum class Frozen2 { Marker };

/**
 * Freezes an object, returning an View bundled with an owned layout and
 * storage.
 */
template <class T,
          class = typename std::enable_if<
              !folly::IsTriviallyCopyable<T>::value>::type,
          class Return = Bundled<typename Layout<T>::View>>
Return freeze(const T& x, Frozen2 = Frozen2::Marker) {
  std::unique_ptr<Layout<T>> layout(new Layout<T>);
  size_t size = LayoutRoot::layout(x, *layout);
  std::unique_ptr<byte[]> storage(new byte[size]);
  folly::MutableByteRange write(storage.get(), size);
  Return ret(ByteRangeFreezer::freeze(*layout, x, write));
  ret.hold(std::move(layout));
  ret.hold(std::move(storage));
  return ret;
};


/**
 * Helper for thawing a single field from a view
 */
template <class T, class Layout>
void thawField(ViewPosition self, const Field<T, Layout>& f, T& out) {
  f.layout.thaw(self(f.pos), out);
}

/**
 * Helper for thawing a field holding an optional into a Thrift optional field
 * and corresponding __isset marker.
 */
template <class T>
void thawField(ViewPosition self,
               const Field<folly::Optional<T>>& f,
               T& out,
               bool& isset) {
  folly::Optional<T> opt;
  f.layout.thaw(self(f.pos), opt);
  if (opt) {
    isset = true;
    out = opt.value();
  } else {
    isset = false;
  }
}

}}}

#include <thrift/lib/cpp2/frozen/FrozenTrivial-inl.h>
#include <thrift/lib/cpp2/frozen/FrozenIntegral-inl.h>
#include <thrift/lib/cpp2/frozen/FrozenBool-inl.h>
#include <thrift/lib/cpp2/frozen/FrozenOptional-inl.h>
#include <thrift/lib/cpp2/frozen/FrozenString-inl.h>
#include <thrift/lib/cpp2/frozen/FrozenPair-inl.h>
#include <thrift/lib/cpp2/frozen/FrozenRange-inl.h>
#include <thrift/lib/cpp2/frozen/FrozenOrderedTable-inl.h>
#include <thrift/lib/cpp2/frozen/FrozenHashTable-inl.h>
#include <thrift/lib/cpp2/frozen/FrozenAssociative-inl.h>
#include <thrift/lib/cpp2/frozen/FrozenEnum-inl.h> // depends on Integral
