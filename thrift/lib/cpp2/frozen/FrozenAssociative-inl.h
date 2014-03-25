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

namespace apache { namespace thrift { namespace frozen { namespace detail {

template <class K, class V>
struct KeyExtractor {
  static const K& getKey(const std::pair<const K, V>& pair) {
    return pair.first;
  }
  static typename Layout<K>::View getViewKey(
      const typename Layout<std::pair<const K, V>>::View& pair) {
    return pair.first();
  }
};

template <class K>
struct SelfKey {
  static const K& getKey(const K& item) { return item; }
  static typename Layout<K>::View getViewKey(
      typename Layout<K>::View itemView) {
    return itemView;
  }
};

template <class T,
          class K,
          class V,
          template <class, class, class, class> class Table>
struct MapTableLayout
    : public Table<T, std::pair<const K, V>, KeyExtractor<K, V>, K> {
  typedef Table<T, std::pair<const K, V>, KeyExtractor<K, V>, K> Base;
  typedef MapTableLayout LayoutSelf;

  class View : public Base::View {
   public:
    typedef typename Layout<K>::View key_type;
    typedef typename Layout<V>::View mapped_type;

    View() {}
    View(const LayoutSelf* layout, ViewPosition position)
        : Base::View(layout, position) {}

    mapped_type getDefault(const key_type& key,
                           mapped_type def = mapped_type()) {
      auto found = this->find(key);
      if (found == this->end()) {
        return std::move(def);
      }
      return found->second();
    }

    mapped_type at(const key_type& key) {
      auto found = this->find(key);
      if (found == this->end()) {
        throw std::out_of_range("Key not found");
      }
      return found->second();
    }
  };

  View view(ViewPosition self) const { return View(this, self); }

  virtual void print(std::ostream& os, int level) const {
    Base::print(os, level);
    os << DebugLine(level) << "...viewed as a map";
  }
};

template <class T, class V, template <class, class, class, class> class Table>
struct SetTableLayout : public Table<T, V, SelfKey<V>, V> {
  typedef Table<T, V, SelfKey<V>, V> Base;

  virtual void print(std::ostream& os, int level) const {
    Base::print(os, level);
    os << DebugLine(level) << "...viewed as a set";
  }
};
}

// assumed unique
template <class K, class V>
class VectorAsMap;

template <class K, class V>
struct Layout<VectorAsMap<K, V>> : public detail::MapTableLayout<
                                       VectorAsMap<K, V>,
                                       K,
                                       V,
                                       detail::SortedTableLayout> {};

template <class K, class V, class A>
struct Layout<std::map<K, V, A>> : public detail::MapTableLayout<
                                       std::map<K, V, A>,
                                       K,
                                       V,
                                       detail::SortedTableLayout> {};
// assumed unique
template <class V>
class VectorAsSet;

template <class V>
struct Layout<VectorAsSet<V>> : public detail::SetTableLayout<
                                    VectorAsSet<V>,
                                    V,
                                    detail::SortedTableLayout> {};

template <class V, class A>
struct Layout<std::set<V, A>> : public detail::SetTableLayout<
                                    std::set<V, A>,
                                    V,
                                    detail::SortedTableLayout> {};

template <class K, class V>
class VectorAsHashMap;

template <class K, class V>
struct Layout<VectorAsHashMap<K, V>> : public detail::MapTableLayout<
                                           VectorAsHashMap<K, V>,
                                           K,
                                           V,
                                           detail::HashTableLayout> {};

template <class K, class V, class A>
struct Layout<std::unordered_map<K, V, A>> : public detail::MapTableLayout<
                                                 std::unordered_map<K, V, A>,
                                                 K,
                                                 V,
                                                 detail::HashTableLayout> {};

template <class V>
class VectorAsHashSet;

template <class V>
struct Layout<VectorAsHashSet<V>> : public detail::SetTableLayout<
                                        VectorAsHashSet<V>,
                                        V,
                                        detail::HashTableLayout> {};

template <class V, class A>
struct Layout<std::unordered_set<V, A>> : public detail::SetTableLayout<
                                              std::unordered_set<V, A>,
                                              V,
                                              detail::HashTableLayout> {};
}
}
}
