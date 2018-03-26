/*
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
#ifndef THRIFT_COMPILER_PY_CONV_H
#define THRIFT_COMPILER_PY_CONV_H

#include <map>
#include <vector>

#include <boost/python.hpp>
#include <boost/python/suite/indexing/map_indexing_suite.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

namespace {
using namespace boost::python;
using namespace boost::python::api;
} // namespace

namespace thrift {
namespace compiler {
namespace py {
namespace conv {

/**
 * Boilerplate that enables automatic exposure of vector<T*> to python
 */
template <class T>
struct indexPtrVec;

/**
 * Boilerplate that enables automatic exposure of vector<T> to python
 */
template <class T>
struct indexVec;

template <class T, class U>
struct indexMap;

/**
 * Template that allows for shorter code to static_cast
 */
template <class T, class U>
const T& TO(const U& from);

} // namespace conv
} // namespace py
} // namespace compiler
} // namespace thrift

#include "thrift/compiler/py/conv.tcc"

#endif // THRIFT_COMPILER_PY_CONV_H
