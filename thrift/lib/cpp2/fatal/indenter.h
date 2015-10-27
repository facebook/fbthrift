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
#ifndef THRIFT_FATAL_INDENTER_H_
#define THRIFT_FATAL_INDENTER_H_ 1

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <cassert>

namespace apache { namespace thrift {

/**
 * Wraps a stream and provides facilities for automatically indenting output.
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <typename OutputStream>
class indenter {
  struct scope {
    explicit scope(indenter *out): out_(out) {}
    scope(scope const &) = delete;
    scope(scope &&rhs) noexcept: out_(rhs.out_) { rhs.out_ = nullptr; }

    ~scope() {
      if (out_) {
        out_->pop();
      }
    }

    scope push() { return out_->push(); }

    scope &skip() {
      out_->skip();
      return *this;
    }

    template <typename U>
    indenter &operator <<(U &&value) {
      return (*out_) << std::forward<U>(value);
    }

    scope &newline() {
      out_->newline();
      return *this;
    }

  private:
    indenter *out_;
  };

public:
  template <typename UOutputStream, typename Indentation>
  indenter(UOutputStream &&out, Indentation &&indentation):
    out_(out)
  {
    indentation_.reserve(4);
    indentation_.emplace_back();
    indentation_.emplace_back(std::forward<Indentation>(indentation));
  }

  scope push() {
    ++level_;

    assert(indentation_.size() > 1);
    indentation_.emplace_back(indentation_.back() + indentation_[1]);
    assert(level_ < indentation_.size());

    return scope(this);
  }

  void pop() {
    assert(level_ > 0);
    --level_;
  }

  indenter &skip() {
    indent_ = false;
    return *this;
  }

  template <typename U>
  indenter &operator <<(U &&value) {
    if (indent_) {
      assert(level_ < indentation_.size());
      out_ << indentation_[level_];
      indent_ = false;
    }

    out_ << std::forward<U>(value);
    return *this;
  }

  indenter &newline() {
    out_ << '\n';
    indent_ = true;
    return *this;
  }

private:
  OutputStream &out_;
  std::vector<std::string> indentation_;
  std::size_t level_ = 0;
  bool indent_ = false;
};

template <typename OutputStream, typename Indentation>
indenter<typename std::decay<OutputStream>::type> make_indenter(
  OutputStream &&out,
  Indentation &&indentation
) {
  return {out, std::forward<Indentation>(indentation)};
}

}} // apache::thrift

#endif // THRIFT_FATAL_INDENTER_H_
