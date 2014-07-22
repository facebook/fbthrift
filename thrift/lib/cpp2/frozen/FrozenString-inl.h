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

namespace apache {
namespace thrift {
namespace frozen {
namespace detail {

/**
 * for contiguous, blittable ranges
 */
template <class T, class Item>
struct StringLayout : public LayoutBase {
  Field<size_t> distance;
  Field<size_t> count;

  StringLayout()
      : LayoutBase(typeid(T)), distance(1, "distance"), count(2, "count") {}

  FieldPosition layout(LayoutRoot& root, const T& o, LayoutPosition self) {
    size_t n = o.size();
    size_t dist = root.layoutBytesDistance(self.start, n * sizeof(Item));
    FieldPosition pos = startFieldPosition();
    pos = root.layoutField(self, pos, distance, dist);
    pos = root.layoutField(self, pos, count, n);
    return pos;
  }

  void freeze(FreezeRoot& root, const T& o, FreezePosition self) const {
    size_t n = o.size();
    folly::MutableByteRange range;
    size_t dist;
    root.appendBytes(self.start, n * sizeof(Item), range, dist);
    root.freezeField(self, distance, dist);
    root.freezeField(self, count, n);
    std::copy(o.begin(), o.end(), reinterpret_cast<Item*>(range.begin()));
  }

  void thaw(ViewPosition self, T& out) const {
    auto range = view(self);
    out.resize(range.size());
    std::copy(range.begin(), range.end(), out.begin());
  }

  typedef folly::Range<const Item*> View;

  View view(ViewPosition self) const {
    View range;
    size_t dist, n;
    thawField(self, count, n);
    if (n) {
      thawField(self, distance, dist);
      const byte* read = self.start + dist;
      range.reset(reinterpret_cast<const Item*>(read), n);
    }
    return range;
  }

  void print(std::ostream& os, int level) const {
    LayoutBase::print(os, level);
    os << "string of " << folly::demangle(type.name());
    distance.print(os, level + 1);
    count.print(os, level + 1);
  }

  void clear() final {
    LayoutBase::clear();
    distance.clear();
    count.clear();
  }

  void save(schema::Schema& schema, schema::Layout& layout) const final {
    LayoutBase::save(schema, layout);
    distance.save(schema, layout);
    count.save(schema, layout);
  }

  void load(const schema::Schema& schema,
            const schema::Layout& layout) final {
    LayoutBase::load(schema, layout);
    distance.load(schema, layout);
    count.load(schema, layout);
  }

  static size_t hash(const View& v) {
    return folly::hash::fnv64_buf(v.begin(), sizeof(Item) * v.size());
  }
};

template <class T>
struct IsString {
  enum {
    value = std::is_same<typename std::decay<T>::type, std::string>::value ||
            std::is_same<typename std::decay<T>::type, folly::fbstring>::value
  };
};

} // detail

template <class T>
struct Layout<T,
              typename std::enable_if<detail::IsString<T>::value>::
                  type> : detail::StringLayout<typename std::decay<T>::type,
                                               typename T::value_type> {};
}
}
}
