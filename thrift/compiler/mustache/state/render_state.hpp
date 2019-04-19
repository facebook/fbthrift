#pragma once

#include <memory>

#include "token.hpp"

namespace mstch {

class render_context;

class render_state {
 public:
  virtual ~render_state() {}
  virtual std::string render(render_context& context, const token& token) = 0;
};

}
