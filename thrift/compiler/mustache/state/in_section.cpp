#include "in_section.hpp"
#include "outside_section.hpp"
#include "visitor/is_node_empty.hpp"
#include "visitor/render_section.hpp"

using namespace mstch;

in_section::in_section(type type, const token& start_token)
    : m_type(type), m_start_token(start_token), m_skipped_openings(0) {}

std::string in_section::render(render_context& ctx, const token& token) {
  if (token.token_type() == token::type::section_close)
    if (token.name() == m_start_token.name() && m_skipped_openings == 0) {
      auto& node = ctx.get_node(m_start_token.name());
      std::string out;

      if (m_type == type::normal && !visit(is_node_empty(), node))
        out =
            visit(render_section(ctx, m_section, m_start_token.delims()), node);
      else if (m_type == type::inverted && visit(is_node_empty(), node))
        out = render_context::push(ctx).render(m_section);

      ctx.set_state<outside_section>();
      return out;
    } else
      m_skipped_openings--;
  else if (
      token.token_type() == token::type::inverted_section_open ||
      token.token_type() == token::type::section_open)
    m_skipped_openings++;

  m_section << token;
  return "";
}
