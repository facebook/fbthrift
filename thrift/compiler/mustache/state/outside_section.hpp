#pragma once

#include "render_state.hpp"

namespace mstch {

class outside_section: public render_state {
 public:
  std::string render(render_context& context, const token& token) override;
};

}
