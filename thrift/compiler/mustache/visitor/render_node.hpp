
#pragma once

#include <sstream>
#include <boost/variant/static_visitor.hpp>

#include "render_context.hpp"
#include "mstch/mstch.hpp"
#include "utils.hpp"

namespace mstch {

class render_node: public boost::static_visitor<std::string> {
 public:
  enum class flag { none, escape_html };
  render_node(render_context& ctx, flag p_flag = flag::none):
      m_ctx(ctx), m_flag(p_flag)
  {
  }

  template<class T>
  std::string operator()(const T&) const {
    return "";
  }

  std::string operator()(const int& value) const {
    return std::to_string(value);
  }

  std::string operator()(const double& value) const {
    std::stringstream ss;
    ss << value;
    return ss.str();
  }

  std::string operator()(const bool& value) const {
    return value ? "true" : "false";
  }

  std::string operator()(const lambda& value) const {
    template_type interpreted{value([this](const mstch::node& n) {
      return visit(render_node(m_ctx), n);
    })};
    auto rendered = render_context::push(m_ctx).render(interpreted);
    return (m_flag == flag::escape_html) ? html_escape(rendered) : rendered;
  }

  std::string operator()(const std::string& value) const {
    return (m_flag == flag::escape_html) ? html_escape(value) : value;
  }

 private:
  render_context& m_ctx;
  flag m_flag;
};

}
