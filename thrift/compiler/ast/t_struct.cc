/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <thrift/compiler/ast/t_struct.h>

namespace apache {
namespace thrift {
namespace compiler {

auto t_struct::get_mixins_and_members() const -> std::vector<mixin_member> {
  std::vector<mixin_member> ret;
  get_mixins_and_members_impl(nullptr, ret);
  return ret;
}

void t_struct::get_mixins_and_members_impl(
    t_field* top_level_mixin,
    std::vector<mixin_member>& out) const {
  for (auto* member : get_members()) {
    if (member->is_mixin()) {
      assert(member->get_type()->get_true_type()->is_struct());
      auto mixin_struct =
          static_cast<const t_struct*>(member->get_type()->get_true_type());
      auto mixin = top_level_mixin ? top_level_mixin : member;

      // import members from mixin field
      for (auto* member_from_mixin : mixin_struct->get_members()) {
        out.push_back({mixin, member_from_mixin});
      }

      // import members from nested mixin field
      mixin_struct->get_mixins_and_members_impl(mixin, out);
    }
  }
}

} // namespace compiler
} // namespace thrift
} // namespace apache
