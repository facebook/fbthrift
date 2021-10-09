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

#include <string>
#include <vector>

#include <folly/portability/GTest.h>

#include <thrift/compiler/generate/t_mstch_generator.h>

namespace apache {
namespace thrift {
namespace compiler {

TEST(t_mstch_generator_test, cacheLeaks) {
  class leaky_program : public mstch_program {
   public:
    leaky_program(
        t_program const* program,
        std::shared_ptr<mstch_generators const> generators,
        std::shared_ptr<mstch_cache> cache,
        ELEMENT_POSITION pos,
        int* object_count)
        : mstch_program(program, generators, cache, pos),
          object_count_(object_count) {
      (*object_count_) += 1;
    }
    virtual ~leaky_program() override { (*object_count_)--; }

   private:
    int* object_count_;
  };

  class leaky_program_generator : public program_generator {
   public:
    explicit leaky_program_generator(int* object_count_)
        : object_count_(object_count_) {}

    virtual std::shared_ptr<mstch_base> generate(
        t_program const* program,
        std::shared_ptr<mstch_generators const> generators,
        std::shared_ptr<mstch_cache> cache,
        ELEMENT_POSITION pos,
        int32_t /*index*/) const override {
      return std::make_shared<leaky_program>(
          program, generators, cache, pos, object_count_);
    }

   private:
    int* object_count_;
  };

  class leaky_generator : public t_mstch_generator {
   public:
    leaky_generator(t_program* program, int* object_count)
        : t_mstch_generator(program, t_generation_context(), ".", {}),
          object_count_(object_count) {}

    void generate_program() override {
      generators_->program_generator_ =
          std::make_unique<leaky_program_generator>(object_count_);
      std::shared_ptr<mstch_base> prog = cached_program(get_program());
    }

   private:
    int* object_count_;
  };

  int object_count = 0;
  {
    auto program = std::make_unique<t_program>("my_leak.thrift");
    leaky_generator generator(program.get(), &object_count);
    generator.generate_program();
  }

  EXPECT_EQ(object_count, 0);
}

} // namespace compiler
} // namespace thrift
} // namespace apache
