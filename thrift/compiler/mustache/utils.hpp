#pragma once

#include <string>
#include <boost/variant/apply_visitor.hpp>

namespace mstch {

using citer = std::string::const_iterator;
using criter = std::string::const_reverse_iterator;

citer first_not_ws(citer begin, citer end);
citer first_not_ws(criter begin, criter end);
std::string html_escape(const std::string& str);
criter reverse(citer it);

template<class... Args>
auto visit(Args&&... args) -> decltype(boost::apply_visitor(
    std::forward<Args>(args)...))
{
  return boost::apply_visitor(std::forward<Args>(args)...);
}

}
