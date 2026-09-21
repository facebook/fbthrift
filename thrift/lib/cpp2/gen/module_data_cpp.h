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

#pragma once

#include <folly/folly-config.h>

#if defined(__GNUC__) && defined(__linux__) && !FOLLY_MOBILE
// These attributes are applied to the static data members to ensure that they
// are not stripped from the compiled binary, in order to keep them available
// for use by debuggers at runtime.
//
// The "used" attribute is required to ensure the compiler always emits unused
// data.
//
// The "section" attribute is required to stop the linker from stripping used
// data. It works by forcing all of the data members (both used and unused ones)
// into the same section. As the linker strips data on a per-section basis, it
// is then unable to remove unused data without also removing used data.
// This has a similar effect to the "retain" attribute, but works with older
// toolchains.
//
// The section must live in RELRO (.data.rel.ro), not .rodata: some members
// hold string_views (pointers needing relocations), so in -fPIC/-fPIE builds
// the compiler would make a .rodata section writable, and the linker would
// merge it into the text segment - producing a single RWX LOAD segment.
// That breaks aarch64 with BTI notes (SIGSEGV in _dl_setup_hash) and trips
// linkers with --error-rwx-segments (Fedora default).
#define THRIFT_DATA_MEMBER \
  [[gnu::used]] [[gnu::section(".data.rel.ro.thrift.data")]]
#else
#define THRIFT_DATA_MEMBER
#endif
