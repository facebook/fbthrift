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

#pragma once

#include <thrift/compiler/parse/t_doc.h>

#include <map>
#include <string>

/**
 * class t_type
 *
 * Generic representation of any parsed element that can support annotations
 */
class t_annotated : public t_doc {
 public:
  virtual ~t_annotated() {}

  std::map<std::string, std::string> annotations_;
};
