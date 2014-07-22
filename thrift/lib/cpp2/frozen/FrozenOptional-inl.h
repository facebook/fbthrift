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
 * Layout specialization for Optional<T>, which is used in the codegen for
 * optional fields. Just a boolean and a value.
 */
template <class T>
struct OptionalLayout : public LayoutBase {
  Field<bool> isset;
  Field<T> value;

  OptionalLayout() : LayoutBase(typeid(T)), isset(1), value(2) {}

  FieldPosition layout(LayoutRoot& root,
                       const folly::Optional<T>& o,
                       LayoutPosition self) {
    FieldPosition pos = startFieldPosition();
    pos = root.layoutField(self, pos, isset, o.hasValue());
    if (o) {
      pos = root.layoutField(self, pos, value, o.value());
    }
    return pos;
  }

  FieldPosition layout(LayoutRoot& root, const T& o, LayoutPosition self) {
    FieldPosition pos = startFieldPosition();
    pos = root.layoutField(self, pos, isset, true);
    pos = root.layoutField(self, pos, value, o);
    return pos;
  }

  void freeze(FreezeRoot& root,
              const folly::Optional<T>& o,
              FreezePosition self) const {
    root.freezeField(self, isset, o.hasValue());
    if (o) {
      root.freezeField(self, value, o.value());
    }
  }

  void freeze(FreezeRoot& root, const T& o, FreezePosition self) const {
    root.freezeField(self, isset, true);
    root.freezeField(self, value, o);
  }

  void thaw(ViewPosition self, folly::Optional<T>& out) const {
    bool set;
    thawField(self, isset, set);
    if (set) {
      out.emplace();
      thawField(self, value, out.value());
    }
  }

  typedef folly::Optional<typename Layout<T>::View> View;

  View view(ViewPosition self) const {
    View v;
    bool set;
    thawField(self, isset, set);
    if (set) {
      v.assign(value.layout.view(self(value.pos)));
    }
    return v;
  }

  void print(std::ostream& os, int level) const final {
    LayoutBase::print(os, level);
    os << "optional " << folly::demangle(type.name());
    isset.print(os, "isset", level + 1);
    value.print(os, "value", level + 1);
  }

  void clear() final {
    isset.clear();
    value.clear();
  }

  void save(schema::Schema& schema, schema::Layout& layout) const final {
    LayoutBase::save(schema, layout);
    isset.save(schema, layout);
    value.save(schema, layout);
  }

  void load(const schema::Schema& schema,
            const schema::Layout& layout) final {
    LayoutBase::load(schema, layout);
    isset.load(schema, layout);
    value.load(schema, layout);
  }
};
}

template <class T>
struct Layout<folly::Optional<T>> : public detail::OptionalLayout<T> {};
}
}
}
