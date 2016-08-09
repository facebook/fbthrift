/*
 * Copyright 2016 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef T_MSTCH_GENERATOR_H
#define T_MSTCH_GENERATOR_H

#include <thrift/compiler/generate/t_generator.h>

class t_mstch_generator : public t_generator {
public:
  t_mstch_generator(t_program *program,
                    std::string language,
                    const std::map<std::string, std::string>& parsed_options,
                    const std::string& option_string);

  virtual void generate_program(void) override;
};

#endif // T_MSTCH_GENERATOR_H
