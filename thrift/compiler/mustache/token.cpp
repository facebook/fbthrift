#include "token.hpp"
#include "utils.hpp"

using namespace mstch;

token::type token::token_info(char c) {
  switch (c) {
    case '>': return type::partial;
    case '^': return type::inverted_section_open;
    case '/': return type::section_close;
    case '&': return type::unescaped_variable;
    case '#': return type::section_open;
    case '!': return type::comment;
    default: return type::variable;
  }
}

token::token(const std::string& str, std::size_t left, std::size_t right):
    m_raw(str), m_eol(false), m_ws_only(false)
{
  if (left != 0 && right != 0) {
    if (str[left] == '=' && str[str.size() - right - 1] == '=') {
      m_type = type::delimiter_change;
    } else if (str[left] == '{' && str[str.size() - right - 1] == '}') {
      m_type = type::unescaped_variable;
      m_name = {first_not_ws(str.begin() + left + 1, str.end() - right),
          first_not_ws(str.rbegin() + 1 + right, str.rend() - left) + 1};
    } else {
      auto c = first_not_ws(str.begin() + left, str.end() - right);
      m_type = token_info(*c);
      if (m_type != type::variable)
        c = first_not_ws(c + 1, str.end() - right);
      m_name = {c, first_not_ws(str.rbegin() + right, str.rend() - left) + 1};
      m_delims = {{str.begin(), str.begin() + left},
          {str.end() - right, str.end()}};
    }
  } else {
    m_type = type::text;
    m_eol = (str.size() > 0 && str[str.size() - 1] == '\n');
    m_ws_only = (str.find_first_not_of(" \r\n\t") == std::string::npos);
  }
}
