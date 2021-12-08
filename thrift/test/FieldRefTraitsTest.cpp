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

#include <thrift/lib/cpp2/FieldRefTraits.h>
#include <thrift/test/testset/gen-cpp2/testset_types.h>

using namespace ::apache::thrift::detail;

namespace apache::thrift::test::testset {
namespace {

using Unqualified = decltype(struct_list_i32{}.field_1_ref());
using Optional = decltype(struct_optional_list_i32{}.field_1_ref());
using Required = decltype(struct_required_list_i32{}.field_1_ref());
using Box = decltype(struct_optional_list_i32_box{}.field_1_ref());
using Union = decltype(union_list_i32{}.field_1_ref());
using Unique = std::remove_reference_t<decltype(
    struct_optional_list_i32_cpp_ref{}.field_1_ref())>;
using Shared = std::remove_reference_t<decltype(
    struct_optional_list_i32_shared_cpp_ref{}.field_1_ref())>;

static_assert(is_field_ref<Unqualified>::value);
static_assert(!is_field_ref<Optional>::value);
static_assert(!is_field_ref<Required>::value);
static_assert(!is_field_ref<Box>::value);
static_assert(!is_field_ref<Union>::value);
static_assert(!is_field_ref<Unique>::value);
static_assert(!is_field_ref<Shared>::value);

static_assert(!is_optional_field_ref<Unqualified>::value);
static_assert(is_optional_field_ref<Optional>::value);
static_assert(!is_optional_field_ref<Required>::value);
static_assert(!is_optional_field_ref<Box>::value);
static_assert(!is_optional_field_ref<Union>::value);
static_assert(!is_optional_field_ref<Unique>::value);
static_assert(!is_optional_field_ref<Shared>::value);

static_assert(!is_required_field_ref<Unqualified>::value);
static_assert(!is_required_field_ref<Optional>::value);
static_assert(is_required_field_ref<Required>::value);
static_assert(!is_required_field_ref<Box>::value);
static_assert(!is_required_field_ref<Union>::value);
static_assert(!is_required_field_ref<Unique>::value);
static_assert(!is_required_field_ref<Shared>::value);

static_assert(!is_optional_boxed_field_ref<Unqualified>::value);
static_assert(!is_optional_boxed_field_ref<Optional>::value);
static_assert(!is_optional_boxed_field_ref<Required>::value);
static_assert(is_optional_boxed_field_ref<Box>::value);
static_assert(!is_optional_boxed_field_ref<Union>::value);
static_assert(!is_optional_boxed_field_ref<Unique>::value);
static_assert(!is_optional_boxed_field_ref<Shared>::value);

static_assert(!is_union_field_ref<Unqualified>::value);
static_assert(!is_union_field_ref<Optional>::value);
static_assert(!is_union_field_ref<Required>::value);
static_assert(!is_union_field_ref<Box>::value);
static_assert(is_union_field_ref<Union>::value);
static_assert(!is_union_field_ref<Unique>::value);
static_assert(!is_union_field_ref<Shared>::value);

static_assert(!is_unique_ptr<Unqualified>::value);
static_assert(!is_unique_ptr<Optional>::value);
static_assert(!is_unique_ptr<Required>::value);
static_assert(!is_unique_ptr<Box>::value);
static_assert(!is_unique_ptr<Union>::value);
static_assert(is_unique_ptr<Unique>::value);
static_assert(!is_unique_ptr<Shared>::value);

static_assert(!is_shared_ptr<Unqualified>::value);
static_assert(!is_shared_ptr<Optional>::value);
static_assert(!is_shared_ptr<Required>::value);
static_assert(!is_shared_ptr<Box>::value);
static_assert(!is_shared_ptr<Union>::value);
static_assert(!is_shared_ptr<Unique>::value);
static_assert(is_shared_ptr<Shared>::value);

static_assert(!is_shared_or_unique_ptr<Unqualified>::value);
static_assert(!is_shared_or_unique_ptr<Optional>::value);
static_assert(!is_shared_or_unique_ptr<Required>::value);
static_assert(!is_shared_or_unique_ptr<Box>::value);
static_assert(!is_shared_or_unique_ptr<Union>::value);
static_assert(is_shared_or_unique_ptr<Unique>::value);
static_assert(is_shared_or_unique_ptr<Shared>::value);

} // namespace
} // namespace apache::thrift::test::testset
