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
#ifndef THRIFT_PY_COMPILER_H
#define THRIFT_PY_COMPILER_H

// boost.python
#include <boost/python.hpp>
#include <boost/python/call.hpp>
#include <boost/python/object.hpp>
#include <boost/python/stl_iterator.hpp>

// boilerplate that enables quick definition of wrappers in boost::python
#include <thrift/compiler/common.h>
#include <thrift/compiler/py/conv.h>

namespace {
using boost::noncopyable;
using namespace boost::python;
using namespace boost::python::api;
using namespace thrift::compiler::py::conv;
} // namespace

extern unique_ptr<t_base_type> g_type_void_sptr;

namespace thrift {
namespace compiler {
namespace py {

void process(const dict& params, const object& generate_callback);

typedef boost::python::tuple pyTuple;

// use for const std::string&
typedef return_value_policy<copy_const_reference> policy_ccr;
// use for T*
typedef return_value_policy<reference_existing_object> policy_reo;
// use for T* with automatic disposal
typedef return_value_policy<manage_new_object> policy_mno;
// use for [const] T&
typedef return_internal_reference<> policy_rir;
// hmmm, maybe not use this unless you know why you need it
typedef return_value_policy<return_by_value> policy_rbv;

/**
 * In the code there are several places where we compare programs and constants.
 * However, comparing the Python wrappers to t_program* that the
 * return_existing_object policy returns won't work. Thus we have to implement
 * our own comparison operators and hardcode them into the python class.
 */
bool t_program_operatorEq(const t_program* self, const t_program* rhs);
bool t_program_operatorNe(const t_program* self, const t_program* rhs);

bool t_const_operatorEq(const t_const* self, const t_const* rhs);
bool t_const_operatorNe(const t_const* self, const t_const* rhs);

} // namespace py
} // namespace compiler
} // namespace thrift

// the Boost::Python module named "frontend" which exposes all the types and
// the process() function
#include "thrift/compiler/py/py_frontend.tcc"

#endif // THRIFT_PY_COMPILER_H
