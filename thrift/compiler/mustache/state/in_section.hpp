#pragma once

#include <sstream>
#include <vector>

#include "render_state.hpp"
#include "template_type.hpp"

namespace mstch {

class in_section: public render_state {
 public:
  enum class type { inverted, normal };
  in_section(type type, const token& start_token);
  std::string render(render_context& context, const token& token) override;

 private:
  const type m_type;
  const token& m_start_token;
  template_type m_section;
  int m_skipped_openings;
};

}
