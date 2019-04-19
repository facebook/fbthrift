#pragma once

#include <boost/variant/static_visitor.hpp>

#include "mstch/mstch.hpp"
#include "has_token.hpp"

namespace mstch {

class get_token: public boost::static_visitor<const mstch::node&> {
 public:
  get_token(const std::string& token, const mstch::node& node):
      m_token(token), m_node(node)
  {
  }

  template<class T>
  const mstch::node& operator()(const T&) const {
    return m_node;
  }

  const mstch::node& operator()(const map& map) const {
    return map.at(m_token);
  }

  const mstch::node& operator()(const std::shared_ptr<object>& object) const {
    return object->at(m_token);
  }

 private:
  const std::string& m_token;
  const mstch::node& m_node;
};

}
