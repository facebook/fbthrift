#include "utils.hpp"
#include "mstch/mstch.hpp"

mstch::citer mstch::first_not_ws(mstch::citer begin, mstch::citer end) {
  for (auto it = begin; it != end; ++it)
    if (*it != ' ') return it;
  return end;
}

mstch::citer mstch::first_not_ws(mstch::criter begin, mstch::criter end) {
  for (auto rit = begin; rit != end; ++rit)
    if (*rit != ' ') return --(rit.base());
  return --(end.base());
}

mstch::criter mstch::reverse(mstch::citer it) {
  return std::reverse_iterator<mstch::citer>(it);
}

std::string mstch::html_escape(const std::string& str) {
  if (mstch::config::escape)
    return mstch::config::escape(str);

  std::string out;
  citer start = str.begin();

  auto add_escape = [&out, &start](const std::string& escaped, citer& it) {
    out += std::string{start, it} + escaped;
    start = it + 1;
  };

  for (auto it = str.begin(); it != str.end(); ++it)
    switch (*it) {
      case '&': add_escape("&amp;", it); break;
      case '\'': add_escape("&#39;", it); break;
      case '"': add_escape("&quot;", it); break;
      case '<': add_escape("&lt;", it); break;
      case '>': add_escape("&gt;", it); break;
      case '/': add_escape("&#x2F;", it); break;
      default: break;
    }

  return out + std::string{start, str.end()};
}
