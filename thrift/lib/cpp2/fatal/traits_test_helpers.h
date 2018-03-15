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

#ifndef THRIFT_FATAL_TRAITS_TEST_HELPERS_H
#define THRIFT_FATAL_TRAITS_TEST_HELPERS_H

namespace apache {
namespace thrift {

/**
 * Test procedure for `thrift_list_traits` specializations of lists that
 * resemble standard containers (say, provide `begin()` and `end()`).
 *
 * Say there's a custom list type `MyList` for which a specialization of
 * `thrift_list_traits` has been provided, simply call this function to test
 * such specialization, passing the custom list type as a parameter:
 *
 *  test_thrift_list_traits<MyList>();
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <typename T>
void test_thrift_list_traits();

/**
 * Test procedure for `thrift_string_traits` specializations of strings that
 * resemble standard containers (say, provide `begin()` and `end()`).
 *
 * Say there's a custom string type `MyString` for which a specialization of
 * `thrift_string_traits` has been provided, simply call this function to test
 * such specialization, passing the custom string type as a parameter:
 *
 *  test_thrift_string_traits<MyString>();
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <typename T>
void test_thrift_string_traits();

/**
 * Test procedure for `thrift_set_traits` specializations of sets that
 * resemble standard containers (say, provide `begin()` and `end()`).
 *
 * Say there's a custom set type `MySet` for which a specialization of
 * `thrift_set_traits` has been provided, simply call this function to test
 * such specialization, passing the custom set type as a parameter:
 *
 *  test_thrift_set_traits<MySet>();
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <typename T>
void test_thrift_set_traits();

/**
 * Test procedure for `thrift_map_traits` specializations of maps that
 * resemble standard containers (say, provide `begin()` and `end()`).
 *
 * Say there's a custom map type `MyMap` for which a specialization of
 * `thrift_map_traits` has been provided, simply call this function to test
 * such specialization, passing the custom map type as a parameter:
 *
 *  test_thrift_map_traits<MyMap>();
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */
template <typename T>
void test_thrift_map_traits();

} // namespace thrift
} // namespace apache

#include <thrift/lib/cpp2/fatal/internal/traits_test_helpers-inl.h>

#endif // THRIFT_FATAL_TRAITS_TEST_HELPERS_H
