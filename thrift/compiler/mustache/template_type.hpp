#pragma once

#include <string>
#include <vector>

#include "token.hpp"
#include "utils.hpp"

namespace mstch {

class template_type {
 public:
  template_type() = default;
  template_type(const std::string& str);
  template_type(const std::string& str, const delim_type& delims);
  std::vector<token>::const_iterator begin() const { return m_tokens.begin(); }
  std::vector<token>::const_iterator end() const { return m_tokens.end(); }
  void operator<<(const token& token) { m_tokens.push_back(token); }

 private:
  std::vector<token> m_tokens;
  std::string m_open;
  std::string m_close;
  void strip_whitespace();
  void process_text(citer beg, citer end);
  void tokenize(const std::string& tmp);
  void store_prefixes(std::vector<token>::iterator beg);
};

}
