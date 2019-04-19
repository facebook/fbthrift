#pragma once

#include <deque>
#include <list>
#include <sstream>
#include <string>
#include <stack>

#include "mstch/mstch.hpp"
#include "state/render_state.hpp"
#include "template_type.hpp"

namespace mstch {

class render_context {
 public:
  class push {
   public:
    push(render_context& context, const mstch::node& node = {});
    ~push();
    std::string render(const template_type& templt);
   private:
    render_context& m_context;
  };

  render_context(
      const mstch::node& node,
      const std::map<std::string, template_type>& partials);
  const mstch::node& get_node(const std::string& token);
  std::string render(
      const template_type& templt, const std::string& prefix = "");
  std::string render_partial(
      const std::string& partial_name, const std::string& prefix);
  template<class T, class... Args>
  void set_state(Args&& ... args) {
    m_state.top() = std::unique_ptr<render_state>(
        new T(std::forward<Args>(args)...));
  }

 private:
  static const mstch::node null_node;
  const mstch::node& find_node(
      const std::string& token,
      std::list<node const*> current_nodes);
  std::map<std::string, template_type> m_partials;
  std::deque<mstch::node> m_nodes;
  std::list<const mstch::node*> m_node_ptrs;
  std::stack<std::unique_ptr<render_state>> m_state;
};

}
