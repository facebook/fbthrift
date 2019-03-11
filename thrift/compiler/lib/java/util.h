/*
 * Copyright 2019-present Facebook, Inc.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#pragma once

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <sstream>
#include <string>

namespace apache {
namespace thrift {
namespace compiler {
namespace java {

/**
 * Mangles an identifier for use in generated Java. Ported from
 * TemplateContextGenerator.java::mangleJavaName
 * from the java implementation of the swift generator.
 * http://tinyurl.com/z7vocup
 */
std::string mangle_java_name(const std::string& ref, bool capitalize);

/**
 * Mangles an identifier for use in generated Java as a constant.
 * Ported from TemplateContextGenerator.java::mangleJavaConstantName
 * from the java implementation of the swift generator.
 * http://tinyurl.com/z7vocup
 */
std::string mangle_java_constant_name(const std::string& ref);

/**
 * Converts a java package string to the path containing the source for
 * that package. Example: "foo.bar.baz" -> "foo/bar/baz"
 */
std::string package_to_path(std::string package);

} // namespace java
} // namespace compiler
} // namespace thrift
} // namespace apache
