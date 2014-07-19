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
namespace apache { namespace thrift { namespace frozen {
namespace detail {

/**
 * Layout specialization for unique ordered range types which support
 * binary-search based lookup.
 */
template <class T, class Item, class KeyExtractor, class Key = T>
struct SortedTableLayout : public ArrayLayout<T, Item> {
  typedef ArrayLayout<T, Item> Base;
  typedef SortedTableLayout LayoutSelf;

  void thaw(ViewPosition self, T& out) const {
    out.clear();
    auto v = view(self);
    for (auto it = v.begin(); it != v.end(); ++it) {
      out.insert(it.thaw());
    }
  }

  class View : public Base::View {
    typedef typename Layout<Key>::View KeyView;
    typedef typename Layout<Item>::View ItemView;

   public:
    View() {}
    View(const LayoutSelf* layout, ViewPosition position)
        : Base::View(layout, position) {}

    typedef typename Base::View::iterator iterator;

    iterator lower_bound(const KeyView& key) const {
      return std::lower_bound(
          this->begin(), this->end(), key, [](ItemView a, KeyView b) {
            return KeyExtractor::getViewKey(a) < b;
          });
    }

    iterator upper_bound(const KeyView& key) const {
      return std::upper_bound(
          this->begin(), this->end(), key, [](KeyView a, ItemView b) {
            return a < KeyExtractor::getViewKey(b);
          });
    }

    std::pair<iterator, iterator> equal_range(const KeyView& key) const {
      auto begin = lower_bound(key);
      if (KeyExtractor::getViewKey(*begin) == key) {
        return make_pair(begin, begin + 1);
      } else {
        return make_pair(begin, begin);
      }
    }

    iterator find(const KeyView& key) const {
      auto found = lower_bound(key);
      if (KeyExtractor::getViewKey(*found) == key) {
        return found;
      } else {
        return this->end();
      }
    }

    size_t count(const KeyView& key) const {
      return find(key) == this->end() ? 0 : 1;
    }

    T thaw() const {
      T ret;
      static_cast<const SortedTableLayout*>(this->layout_)
          ->thaw(this->position_, ret);
      return ret;
    }
  };

  View view(ViewPosition self) const { return View(this, self); }
};

} // detail
}}}
