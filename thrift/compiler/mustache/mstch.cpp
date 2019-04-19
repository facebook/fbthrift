#include <iostream>

#include "mstch/mstch.hpp"
#include "render_context.hpp"

using namespace mstch;

std::function<std::string(const std::string&)> mstch::config::escape;

std::string mstch::render(
    const std::string& tmplt,
    const node& root,
    const std::map<std::string,std::string>& partials)
{
  std::map<std::string, template_type> partial_templates;
  for (auto& partial: partials)
    partial_templates.insert({partial.first, {partial.second}});

  return render_context(root, partial_templates).render(tmplt);
}
