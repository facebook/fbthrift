#include "outside_section.hpp"

#include "visitor/render_node.hpp"
#include "in_section.hpp"
#include "render_context.hpp"

using namespace mstch;

std::string outside_section::render(
    render_context& ctx, const token& token)
{
  using flag = render_node::flag;
  switch (token.token_type()) {
    case token::type::section_open:
      ctx.set_state<in_section>(in_section::type::normal, token);
      break;
    case token::type::inverted_section_open:
      ctx.set_state<in_section>(in_section::type::inverted, token);
      break;
    case token::type::variable:
      return visit(render_node(ctx, flag::escape_html), ctx.get_node(token.name()));
    case token::type::unescaped_variable:
      return visit(render_node(ctx, flag::none), ctx.get_node(token.name()));
    case token::type::text:
      return token.raw();
    case token::type::partial:
      return ctx.render_partial(token.name(), token.partial_prefix());
    default:
      break;
  }
  return "";
}
