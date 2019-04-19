#pragma once

#include <string>

namespace mstch {

using delim_type = std::pair<std::string, std::string>;

class token {
 public:
  enum class type {
    text, variable, section_open, section_close, inverted_section_open,
    unescaped_variable, comment, partial, delimiter_change
  };
  token(const std::string& str, std::size_t left = 0, std::size_t right = 0);
  type token_type() const { return m_type; };
  const std::string& raw() const { return m_raw; };
  const std::string& name() const { return m_name; };
  const std::string& partial_prefix() const { return m_partial_prefix; };
  const delim_type& delims() const { return m_delims; };
  void partial_prefix(const std::string& p_partial_prefix) {
    m_partial_prefix = p_partial_prefix;
  };
  bool eol() const { return m_eol; }
  void eol(bool eol) { m_eol = eol; }
  bool ws_only() const { return m_ws_only; }

 private:
  type m_type;
  std::string m_name;
  std::string m_raw;
  std::string m_partial_prefix;
  delim_type m_delims;
  bool m_eol;
  bool m_ws_only;
  type token_info(char c);
};

}
