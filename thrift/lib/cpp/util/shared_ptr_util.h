/*
 * Copyright 2017-present Facebook, Inc.
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

#ifndef THRIFT_UTIL_SHARED_PTR_UTIL_H_
#define THRIFT_UTIL_SHARED_PTR_UTIL_H_ 1

#include <boost/type_traits/is_convertible.hpp>
#include <boost/utility/enable_if.hpp>

/**
 * Helper macros to allow function overloading even when using
 * std::shared_ptr.
 *
 * shared_ptr makes overloading really annoying, since shared_ptr defines
 * constructor methods to allow one shared_ptr type to be constructed from any
 * other shared_ptr type.  (Even if it would be a compile error to actually try
 * to instantiate the constructor.)  These macros add an extra argument to the
 * function to cause it to only be instantiated if a pointer of type T is
 * convertible to a pointer of type Y.
 *
 * THRIFT_OVERLOAD_IF should be used in function declarations.
 * THRIFT_OVERLOAD_IF_DEFN should be used in the function definition, if it is
 * defined separately from where it is declared.
 */
#define THRIFT_OVERLOAD_IF_DEFN(T, Y) \
  typename ::boost::enable_if<typename ::boost::is_convertible<T*, Y*>::type, \
                              void*>::type

#define THRIFT_OVERLOAD_IF(T, Y) \
  THRIFT_OVERLOAD_IF_DEFN(T, Y) = nullptr

#endif // THRIFT_UTIL_SHARED_PTR_UTIL_H_
