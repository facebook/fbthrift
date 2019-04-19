#pragma once

#include <boost/variant/static_visitor.hpp>

#include "render_context.hpp"
#include "mstch/mstch.hpp"
#include "utils.hpp"
#include "render_node.hpp"

namespace mstch {

class render_section: public boost::static_visitor<std::string> {
 public:
  enum class flag { none, keep_array };
  render_section(
      render_context& ctx,
      const template_type& section,
      const delim_type& delims,
      flag p_flag = flag::none):
      m_ctx(ctx), m_section(section), m_delims(delims), m_flag(p_flag)
  {
  }

  template<class T>
  std::string operator()(const T& t) const {
    return render_context::push(m_ctx, t).render(m_section);
  }

  std::string operator()(const lambda& fun) const {
    std::string section_str;
    for (auto& token: m_section)
      section_str += token.raw();
    template_type interpreted{fun([this](const mstch::node& n) {
      return visit(render_node(m_ctx), n);
    }, section_str), m_delims};
    return render_context::push(m_ctx).render(interpreted);
  }

  std::string operator()(const array& array) const {
    std::string out;
    if (m_flag == flag::keep_array)
      return render_context::push(m_ctx, array).render(m_section);
    else
      for (auto& item: array)
        out += visit(render_section(
            m_ctx, m_section, m_delims, flag::keep_array), item);
    return out;
  }

 private:
  render_context& m_ctx;
  const template_type& m_section;
  const delim_type& m_delims;
  flag m_flag;
};

}
