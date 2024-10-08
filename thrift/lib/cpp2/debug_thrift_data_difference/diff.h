/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

#include <string>
#include <string_view>

#include <folly/Range.h>

#include <thrift/lib/cpp2/debug_thrift_data_difference/debug.h>
#include <thrift/lib/cpp2/debug_thrift_data_difference/pretty_print.h>

namespace facebook::thrift {

/**
 * A handy callback for `debug_thrift_data_difference()` that outputs a
 * diff-like format to a given stream.
 *
 * See `make_diff_output_callback` for a convenient way to create an instance
 * of this callback.
 */
template <typename Output>
struct diff_output_callback {
  diff_output_callback(Output& out, std::string_view lhs, std::string_view rhs)
      : out_(out), lhs_(lhs), rhs_(rhs) {}

  template <typename Tag, typename T>
  void operator()(
      Tag, T const* lhs, T const* rhs, std::string_view path, std::string_view)
      const {
    out_ << path << ":\n";
    if (lhs) {
      facebook::thrift::pretty_print<Tag>(out_, *lhs, "  ", std::string(lhs_));
      out_ << "\n";
    }
    if (rhs) {
      facebook::thrift::pretty_print<Tag>(out_, *rhs, "  ", std::string(rhs_));
      out_ << "\n";
    }
  }

 private:
  Output& out_;
  std::string_view lhs_;
  std::string_view rhs_;
};

/**
 * A convenient way to create an instance of `diff_output_callback`.
 *
 * Example:
 *
 *  bool const differs = debug_thrift_data_difference(
 *    lhs,
 *    rhs,
 *    make_diff_output_callback(std::cout)
 *  );
 *
 *  EXPECT_TRUE(
 *    debug_thrift_data_difference(
 *      lhs,
 *      rhs,
 *      make_diff_output_callback(LOG(ERROR))
 *    )
 *  );
 */

template <typename Output>
diff_output_callback<Output> make_diff_output_callback(
    Output& output, std::string_view lhs = "- ", std::string_view rhs = "+ ") {
  return diff_output_callback<Output>(output, lhs, rhs);
}

} // namespace facebook::thrift
