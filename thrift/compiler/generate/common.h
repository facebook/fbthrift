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

#include <string>
#include <vector>

/**
 * Split a namespace string using '.' as a token
 */
std::vector<std::string> split_namespace(const std::string& s);

/**
 * TODO: Only used by t_cpp_gen. Move function to a more appropriate
 *       location or remove once it's not used.
 *
 * Backlashes all occurrances of double-quote
 *    "  ->  \"
 */
void escape_quotes_cpp(std::string& s);

/**
 * Removes whitespace from the left and right side of a string
 */
void trim_whitespace(std::string& s);
