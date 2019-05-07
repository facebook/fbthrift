/*

The source code contained in this file is based on the original code by
Daniel Sipka (https://github.com/no1msd/mstch). The original license by Daniel
Sipka can be read below:

The MIT License (MIT)

Copyright (c) 2015 Daniel Sipka

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include "thrift/compiler/mustache/utils.h"
#include "thrift/compiler/mustache/mstch.h"

namespace apache {
namespace thrift {
namespace mstch {

citer first_not_ws(citer begin, citer end) {
  for (auto it = begin; it != end; ++it) {
    if (*it != ' ') {
      return it;
    }
  }
  return end;
}

citer first_not_ws(criter begin, criter end) {
  for (auto rit = begin; rit != end; ++rit) {
    if (*rit != ' ') {
      return --(rit.base());
    }
  }
  return --(end.base());
}

criter reverse(citer it) {
  return std::reverse_iterator<citer>(it);
}

std::string html_escape(const std::string& str) {
  if (config::escape) {
    return config::escape(str);
  }

  std::string out;
  citer start = str.begin();

  auto add_escape = [&out, &start](const std::string& escaped, citer& it) {
    out += std::string{start, it} + escaped;
    start = it + 1;
  };

  for (auto it = str.begin(); it != str.end(); ++it) {
    switch (*it) {
      case '&':
        add_escape("&amp;", it);
        break;
      case '\'':
        add_escape("&#39;", it);
        break;
      case '"':
        add_escape("&quot;", it);
        break;
      case '<':
        add_escape("&lt;", it);
        break;
      case '>':
        add_escape("&gt;", it);
        break;
      case '/':
        add_escape("&#x2F;", it);
        break;
      default:
        break;
    }
  }

  return out + std::string{start, str.end()};
}

} // namespace mstch
} // namespace thrift
} // namespace apache
