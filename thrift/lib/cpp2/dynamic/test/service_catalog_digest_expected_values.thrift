/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

// Expected digest hex strings for cross-language ServiceCatalogDigest
// validation, computed from the services in service_catalog_digest_fixture.thrift.

package "facebook.com/thrift/service_catalog/test"

namespace cpp2 apache.thrift.dynamic.test.service_catalog_digest_expected_values
namespace rust service_catalog_digest_expected_values
namespace py3 apache.thrift.dynamic.test

const string DIGEST_CALCULATOR = "67b2093d5cc3854bf42190c2015177c7c7fb464b4173eef8365ea950d5422ba6";
const string DIGEST_RICH_DESCRIPTOR = "39948ca7cee48589413675e68920de0eb414c56964964983d5fae51c5e9c270f";
const string DIGEST_RICH_DESCRIPTOR_STRUCTURAL = "56aa4e9d792f7ff25d1d4b45b539a20cc00596bd96fa1b4d2815eb2410b34002";
const string DIGEST_PERFORMED_INTERACTION = "724c5fafe81762d7b60b6d2962c2bcca5cf7bfb021fe520261c8efc1257e4cf9";
