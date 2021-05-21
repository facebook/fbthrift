/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <cassert>
#include <initializer_list>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

namespace apache {
namespace thrift {
namespace compiler {

namespace detail {

template <typename Derived, typename T>
class base_view {
 public:
  using element_type = T;
  using value_type = std::remove_cv_t<T>;

  constexpr auto rbegin() const noexcept {
    std::make_reverse_iterator(derived().end());
  }
  constexpr auto rend() const noexcept {
    std::make_reverse_iterator(derived().begin());
  }
  constexpr bool empty() const noexcept { return derived().size() == 0; }

 private:
  Derived& derived() { return *static_cast<Derived*>(this); }
  const Derived& derived() const { return *static_cast<const Derived*>(this); }
};

template <typename Derived, typename T>
class base_span : public base_view<Derived, T> {
  using base = base_view<Derived, T>;

 public:
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using pointer = T*;
  using reference = T&;
  using iterator = pointer;
  using reverse_iterator = std::reverse_iterator<iterator>;

  constexpr base_span(const std::string* data, std::size_t size) noexcept
      : data_(data), size_(size) {}

  // Provided for gtest compatibility
  using const_iterator = iterator;

  constexpr iterator begin() const noexcept { return data_; }
  constexpr iterator end() const noexcept { return data_ + size_; }
  constexpr size_type size() const noexcept { return size_; }
  constexpr reference operator[](size_type pos) const {
    assert(pos < size_);
    return *(data_ + pos);
  }
  constexpr reference front() const { return operator[](0); }
  constexpr reference back() const { return operator[](size_ - 1); }

 private:
  pointer data_;
  size_type size_;
};

} // namespace detail

// An ordered list of the aliases for an annotation.
//
// If two aliases are set on a node, the value for the first alias in the span
// will be used.
//
// Like std::span and std::string_view, this class provides access
// to memory it does not own and must not be accessed after the associated
// data is destroyed.
class alias_span : public detail::base_span<alias_span, const std::string> {
  using base = detail::base_span<alias_span, const std::string>;

 public:
  using base::base;
  /* implicit */ constexpr alias_span(
      std::initializer_list<std::string> name) noexcept
      : alias_span(name.begin(), name.size()) {}
  /* implicit */ constexpr alias_span(const std::string& name) noexcept
      : alias_span(&name, 1) {}
  template <
      typename C = std::vector<std::string>,
      typename =
          decltype(std::declval<const C&>().data() + std::declval<const C&>().size())>
  /* implicit */ alias_span(const C& list)
      : alias_span(list.data(), list.size()) {}
  constexpr alias_span(const alias_span&) noexcept = default;
  constexpr alias_span& operator=(const alias_span&) noexcept = default;
};

/**
 * class t_node
 *
 * Base data structure for every parsed element in
 * a thrift program.
 */
class t_node {
 public:
  virtual ~t_node() = default;

  const std::string& doc() const { return doc_; }
  bool has_doc() const { return has_doc_; }
  void set_doc(const std::string& doc) {
    doc_ = doc;
    has_doc_ = true;
  }

  int lineno() const { return lineno_; }
  void set_lineno(int lineno) { lineno_ = lineno; }

  // The annotations declared directly on this node.
  const std::map<std::string, std::string>& annotations() const {
    return annotations_;
  }

  // Returns true if there exists an annotation with the given name.
  bool has_annotation(alias_span name) const {
    return get_annotation_or_null(name) != nullptr;
  }
  bool has_annotation(const char* name) const {
    return has_annotation(alias_span{name});
  }

  // Returns the pointer to the value of the first annotation found with the
  // given name.
  //
  // If not found returns nullptr.
  const std::string* get_annotation_or_null(alias_span name) const;
  const std::string* get_annotation_or_null(const char* name) const {
    return get_annotation_or_null(alias_span{name});
  }

  // Returns the value of an annotation with the given name.
  //
  // If not found returns the provided default or "".
  template <
      typename T = std::vector<std::string>,
      typename D = const std::string*>
  decltype(auto) get_annotation(
      const T& name, D&& default_value = nullptr) const {
    return annotation_or(
        get_annotation_or_null(alias_span{name}),
        std::forward<D>(default_value));
  }

  void reset_annotations(
      std::map<std::string, std::string> annotations, int last_lineno) {
    annotations_ = std::move(annotations);
    last_annotation_lineno_ = last_lineno;
  }

  void set_annotation(const std::string& key, std::string value = {}) {
    annotations_[key] = std::move(value);
  }

  int last_annotation_lineno() const { return last_annotation_lineno_; }

 protected:
  // t_node is abstract.
  t_node() = default;

  static const std::string kEmptyString;

  template <typename D>
  static std::string annotation_or(const std::string* val, D&& def) {
    if (val != nullptr) {
      return *val;
    }
    return std::forward<D>(def);
  }

  static const std::string& annotation_or(
      const std::string* val, const std::string* def) {
    return val ? *val : (def ? *def : kEmptyString);
  }

  static const std::string& annotation_or(
      const std::string* val, std::string* def) {
    return val ? *val : (def ? *def : kEmptyString);
  }

 private:
  std::string doc_;
  bool has_doc_{false};
  int lineno_{-1};

  std::map<std::string, std::string> annotations_;
  // TODO(afuller): Looks like only this is only used by t_json_generator.
  // Consider removing.
  int last_annotation_lineno_{-1};

  // TODO(afuller): Remove everything below this comment. It is only provideed
  // for backwards compatibility.
 public:
  const std::string& get_doc() const { return doc_; }
  int get_lineno() const { return lineno_; }
};

using t_annotation = std::map<std::string, std::string>::value_type;

// The type to use to hold a list of nodes of type T.
template <typename T>
using node_list = std::vector<std::unique_ptr<T>>;

// A view of a node_list of nodes of type T.
//
// If T is const, only provides const access to the nodes in the
// node_list.
//
// Like std::ranges::view, std::span and std::string_view, this class provides
// access to memory it does not own and must not be accessed after the
// associated node_list is destroyed.
//
// TODO(afuller): When c++20 can be used, switch to using
// std::ranges::transform_view instead.
template <typename T>
class node_list_view : public detail::base_view<node_list_view<T>, T*> {
  using base = detail::base_view<node_list_view<T>, T*>;
  using node_list_type = node_list<std::remove_cv_t<T>>;

 public:
  using size_type = typename node_list_type::size_type;
  using difference_type = typename node_list_type::difference_type;

  // Return by value.
  // This follows the std::ranges::view behavior.
  using value_type = T*;
  using pointer = void;
  using reference = value_type;

  class iterator {
    friend class node_list_view;
    using itr_type = typename node_list_type::const_iterator;
    /* implicit */ constexpr iterator(itr_type itr) : itr_(std::move(itr)) {}

   public:
    using iterator_category = typename itr_type::iterator_category;
    using difference_type = typename itr_type::difference_type;
    using value_type = node_list_view::value_type;
    using pointer = node_list_view::pointer;
    using reference = node_list_view::reference;

    reference operator*() const noexcept { return itr_->get(); }
    reference operator[](difference_type n) const noexcept {
      return itr_[n].get();
    }

    constexpr iterator operator++(int) noexcept { return {itr_++}; }
    constexpr iterator& operator++() noexcept {
      ++itr_;
      return *this;
    }
    constexpr iterator& operator+=(difference_type n) noexcept {
      itr_ += n;
      return *this;
    }
    constexpr iterator& operator-=(difference_type n) noexcept {
      itr_ -= n;
      return *this;
    }

    friend constexpr iterator operator-(
        const iterator& a, const difference_type& n) noexcept {
      return {a.itr_ - n};
    }
    friend constexpr iterator operator+(
        const iterator& a, const difference_type& n) noexcept {
      return {a.itr_ + n};
    }
    friend constexpr iterator operator+(
        const difference_type& n, const iterator& b) noexcept {
      return {n + b.itr_};
    }

   private:
    itr_type itr_;

#define __FBTHRIFT_NODE_SPAN_ITR_FWD_OP(op)                       \
  friend auto operator op(const iterator& a, const iterator& b) { \
    return a.itr_ op b.itr_;                                      \
  }
    __FBTHRIFT_NODE_SPAN_ITR_FWD_OP(-)
    __FBTHRIFT_NODE_SPAN_ITR_FWD_OP(==)
    __FBTHRIFT_NODE_SPAN_ITR_FWD_OP(!=)
    __FBTHRIFT_NODE_SPAN_ITR_FWD_OP(<)
    __FBTHRIFT_NODE_SPAN_ITR_FWD_OP(<=)
    __FBTHRIFT_NODE_SPAN_ITR_FWD_OP(>)
    __FBTHRIFT_NODE_SPAN_ITR_FWD_OP(>=)
#undef __FBTHRIFT_NODE_SPAN_ITR_FWD_OP
  };
  using reverse_iterator = std::reverse_iterator<iterator>;

  /* implicit */ constexpr node_list_view(const node_list_type& list) noexcept
      : list_(&list) {}

  constexpr node_list_view(const node_list_view&) noexcept = default;
  constexpr node_list_view& operator=(const node_list_view&) noexcept = default;

  constexpr T* front() const { return list_->front().get(); }
  constexpr T* back() const { return list_->back().get(); }
  constexpr T* operator[](std::size_t pos) const { return at(pos); }
  constexpr iterator begin() const noexcept { return list_->begin(); }
  constexpr iterator end() const noexcept { return list_->end(); }
  constexpr std::size_t size() const noexcept { return list_->size(); }

  // Create an std::vector with the same contents as this span.
  constexpr std::vector<T*> copy() const noexcept { return {begin(), end()}; }

  // Provided for backwards compatibility with std::vector API.
  using const_iterator = iterator;
  using const_reference = reference;
  using const_pointer = pointer;
  constexpr iterator cbegin() const noexcept { return list_->cbegin(); }
  constexpr iterator cend() const noexcept { return list_->cend(); }
  constexpr T* at(std::size_t pos) const { return list_->at(pos).get(); }

 private:
  const node_list_type* list_;
};

} // namespace compiler
} // namespace thrift
} // namespace apache
