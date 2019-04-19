#include "render_context.hpp"
#include "state/outside_section.hpp"
#include "visitor/get_token.hpp"

using namespace mstch;

const mstch::node render_context::null_node;

render_context::push::push(render_context& context, const mstch::node& node):
    m_context(context)
{
  context.m_nodes.emplace_front(node);
  context.m_node_ptrs.emplace_front(&node);
  context.m_state.push(std::unique_ptr<render_state>(new outside_section));
}

render_context::push::~push() {
  m_context.m_nodes.pop_front();
  m_context.m_node_ptrs.pop_front();
  m_context.m_state.pop();
}

std::string render_context::push::render(const template_type& templt) {
  return m_context.render(templt);
}

render_context::render_context(
    const mstch::node& node,
    const std::map<std::string, template_type>& partials):
    m_partials(partials), m_nodes(1, node), m_node_ptrs(1, &node)
{
  m_state.push(std::unique_ptr<render_state>(new outside_section));
}

const mstch::node& render_context::find_node(
    const std::string& token,
    std::list<node const*> current_nodes)
{
  if (token != "." && token.find('.') != std::string::npos)
    return find_node(token.substr(token.rfind('.') + 1),
        {&find_node(token.substr(0, token.rfind('.')), current_nodes)});
  else
    for (auto& node: current_nodes)
      if (visit(has_token(token), *node))
        return visit(get_token(token, *node), *node);
  return null_node;
}

const mstch::node& render_context::get_node(const std::string& token) {
    return find_node(token, m_node_ptrs);
}

std::string render_context::render(
    const template_type& templt, const std::string& prefix)
{
  std::string output;
  bool prev_eol = true;
  for (auto& token: templt) {
    if (prev_eol && prefix.length() != 0)
      output += m_state.top()->render(*this, {prefix});
    output += m_state.top()->render(*this, token);
    prev_eol = token.eol();
  }
  return output;
}

std::string render_context::render_partial(
    const std::string& partial_name, const std::string& prefix)
{
  return m_partials.count(partial_name) ?
      render(m_partials.at(partial_name), prefix) : "";
}
