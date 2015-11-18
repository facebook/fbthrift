/*
 * Copyright 2015 Facebook, Inc.
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

#include <map>

namespace apache { namespace thrift { namespace test {

template <typename Key, typename Value>
class MapWithNoInsert {
 private:
  using mrep = std::map<Key, Value>;
  using self = MapWithNoInsert<Key, Value>;
 public:
  using key_type = typename mrep::key_type;
  using mapped_type = typename mrep::mapped_type;
  using value_type = typename mrep::value_type;
  using size_type = typename mrep::size_type;

  class const_iterator {
   private:
    using irep = typename mrep::const_iterator;
    using self = const_iterator;
   public:
    explicit const_iterator(irep r) : rep_(r) {}
    const value_type* operator->() const { return &*rep_; }
    self& operator++() { ++rep_; return *this; }
    bool operator!=(const self& that) const { return rep_ != that.rep_; }
   private:
    irep rep_;
  };

  size_type size() const { return rep_.size(); }
  void clear() { rep_.clear(); }
  mapped_type& operator[](const key_type& key) { return rep_[key]; }
  const_iterator begin() const { return const_iterator(rep_.begin()); }
  const_iterator end() const { return const_iterator(rep_.end()); }
  bool operator==(const self& that) const { return rep_ == that.rep_; }
 private:
  mrep rep_;
};

}}}
