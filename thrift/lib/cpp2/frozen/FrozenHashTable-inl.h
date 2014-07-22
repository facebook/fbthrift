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

struct Block {
  uint64_t mask = 0;
  size_t offset = 0;
  static constexpr size_t bits = 64;
};

struct BlockLayout : public LayoutBase {
  typedef Block T;
  typedef BlockLayout LayoutSelf;

  Field<uint64_t, TrivialLayout<uint64_t>> maskField;
  Field<uint64_t> offsetField;

  BlockLayout()
      : LayoutBase(typeid(T)), maskField(1, "mask"), offsetField(2, "offset") {}

  FieldPosition layout(LayoutRoot& root, const T& o, LayoutPosition self);
  void freeze(FreezeRoot& root, const T& o, FreezePosition self) const;
  void print(std::ostream& os, int level) const final;
  void clear() final;
  void save(schema::Schema&, schema::Layout&) const final;
  void load(const schema::Schema&, const schema::Layout&) final;

  struct View : public ViewBase<View, LayoutSelf, T> {
    View() {}
    View(const LayoutSelf* layout, ViewPosition position)
        : ViewBase<View, LayoutSelf, T>(layout, position) {}

    uint64_t mask() const {
      return this->layout_->maskField.layout.view(
          this->position_(this->layout_->maskField.pos));
    }
    uint64_t offset() const {
      return this->layout_->offsetField.layout.view(
          this->position_(this->layout_->offsetField.pos));
    }
  };

  View view(ViewPosition self) const { return View(this, self); }
};
}

template<>
struct Layout<detail::Block>: detail::BlockLayout{};

namespace detail {
/**
 * Layout specialization for range types which support unique hash lookup.
 */
template <class T,
          class Item,
          class KeyExtractor,
          class Key>
struct HashTableLayout : public ArrayLayout<T, Item> {
  typedef ArrayLayout<T, Item> Base;
  Field<std::vector<Block>> sparseTableField;
  typedef Layout<Key> KeyLayout;
  typedef HashTableLayout LayoutSelf;

  HashTableLayout()
      : sparseTableField(4,
                         "sparseTable") // continue field ids from ArrayLayout
  {}

  static size_t blockCount(size_t size) {
    // LF = Load Factor, BPE = bits/entry
    // 1.5 => 66% LF => 3 bpe, 3 probes expected
    // 2.0 => 50% LF => 4 bpe, 2 probes expected
    // 2.5 => 40% LF => 5 bpe, 1.6 probes expected
    return size_t(size * 2.5 + Block::bits - 1) / Block::bits;
  }

  static void buildIndex(const T& coll,
                         std::vector<const Item*>& index,
                         std::vector<Block>& sparseTable) {
    auto blocks = blockCount(coll.size());
    size_t buckets = blocks * Block::bits;
    sparseTable.resize(blocks);
    index.resize(buckets);
    for (auto& item : coll) {
      size_t h = KeyLayout::hash(KeyExtractor::getKey(item));
      h *= 5; // spread out clumped hash values
      for (size_t p = 0; ; h += ++p) { // quadratic probing
        size_t bucket = h % buckets;
        const Item** slot = &index[bucket];
        if (*slot) {
          if (p == buckets) {
            throw std::out_of_range("All buckets full!");
          }
          continue;
        } else {
          *slot = &item;
          break;
        }
      }
    }
    size_t count = 0;
    size_t bucketCount = 0;
    for (size_t blockIndex = 0; blockIndex < blocks; ++blockIndex) {
      Block& block = sparseTable[blockIndex];
      block.offset = count;
      for (int offset = 0; offset < Block::bits; ++offset) {
        if (index[blockIndex * Block::bits + offset]) {
          block.mask |= uint64_t(1) << offset;
          ++count;
        }
      }
    }
  }

  FieldPosition layoutItems(LayoutRoot& root,
                            const T& coll,
                            LayoutPosition self,
                            FieldPosition pos,
                            LayoutPosition write,
                            FieldPosition writeStep) final {
    std::vector<const Item*> index;
    std::vector<Block> sparseTable;
    buildIndex(coll, index, sparseTable);

    pos = root.layoutField(self, pos, this->sparseTableField, sparseTable);

    FieldPosition noField; // not really used
    for (auto& it : index) {
      if (it) {
        root.layoutField(write, noField, this->item, *it);
        write = write(writeStep);
      }
    }

    return pos;
  }

  void freezeItems(FreezeRoot& root,
                   const T& coll,
                   FreezePosition self,
                   FreezePosition write,
                   FieldPosition writeStep) const final {
    std::vector<const Item*> index;
    std::vector<Block> sparseTable;
    buildIndex(coll, index, sparseTable);

    assert(index.empty() == sparseTable.empty());
    root.freezeField(self, this->sparseTableField, sparseTable);

    FieldPosition noField; // not really used
    for (auto& it : index) {
      if (it) {
        root.freezeField(write, this->item, *it);
        write = write(writeStep);
      }
    }
  }

  void thaw(ViewPosition self, T& out) const {
    out.clear();
    auto outIt = std::inserter(out, out.begin());
    auto v = view(self);
    for (auto it = v.begin(); it != v.end(); ++it) {
      *outIt++ = it.thaw();
    }
  }

  void print(std::ostream& os, int level) const override {
    Base::print(os, level);
    sparseTableField.print(os,  level + 1);
  }

  void clear() final {
    Base::clear();
    sparseTableField.clear();
  }

  void save(schema::Schema& schema, schema::Layout& layout) const final {
    Base::save(schema, layout);
    sparseTableField.save(schema, layout);
  }

  void load(const schema::Schema& schema,
            const schema::Layout& layout) final {
    Base::load(schema, layout);
    sparseTableField.load(schema, layout);
  }

  class View : public Base::View {
    typedef typename Layout<Key>::View KeyView;
    typedef typename Layout<Item>::View ItemView;
    typedef typename Layout<std::vector<Block>>::View TableView;

    TableView table_;
   public:
    View() {}
    View(const LayoutSelf* layout, ViewPosition self)
        : Base::View(layout, self),
          table_(layout->sparseTableField.layout.view(
              self(layout->sparseTableField.pos))) {}

    typedef typename Base::View::iterator iterator;

    std::pair<iterator, iterator> equal_range(const KeyView& key) const {
      auto found = find(key);
      if (found != this->end()) {
        return make_pair(found, found + 1);
      } else {
        return make_pair(found, found);
      }
    }

    iterator find(const KeyView& key) const {
      auto h = KeyLayout::hash(key);
      h *= 5; // spread out clumped values
      auto blocks = table_.size();
      auto buckets = blocks * Block::bits;
      for (size_t p = 0; p < buckets; h += ++p) { // quadratic probing
        auto bucket = h % buckets;
        auto major = bucket / Block::bits;
        auto minor = bucket % Block::bits;
        auto block = table_[major];
        auto mask = block.mask();
        auto offset = block.offset();
        for (;;) {
          if (0 == (1 & (mask >> minor))) {
            return this->end();
          }
          size_t subOffset = folly::popcount(mask & ((1ULL << minor) - 1));
          auto index = offset + subOffset;
          auto found = this->begin() + index;
          if (KeyExtractor::getViewKey(*found) == key) {
            return found;
          }
          minor += ++p;
          if (LIKELY(minor < Block::bits)) {
            h += p; // same block shortcut
          } else {
            --p; // undo
            break;
          }
        }
      }
      return this->end();
    }

    size_t count(const KeyView& key) const {
      return find(key) == this->end() ? 0 : 1;
    }

    T thaw() const {
      T ret;
      static_cast<const HashTableLayout*>(this->layout_)
          ->thaw(this->position_, ret);
      return ret;
    }
  };

  View view(ViewPosition self) const { return View(this, self); }
};
}}}}
