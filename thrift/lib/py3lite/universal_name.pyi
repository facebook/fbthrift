# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from enum import Enum

class UniversalHashAlgorithm(Enum):
    Sha2_256: UniversalHashAlgorithm = ...

# Validates that uri is a valid universal name uri of the form:
# {domain}/{path}. For example: facebook.com/thrift/Value.
#
# The scheme "fbthrift://"" is implied and not included in the uri.
#
# Throws ValueError on failure.
def validate_universal_name(uri: str) -> None: ...

# Validates that the given type hash meets the size requirements.
#
# Throws ValueError on failure.
def validate_universal_hash(
    alg: UniversalHashAlgorithm, universal_hash: bytes, min_hash_bytes: int
) -> None: ...

# Validates that the given type hash bytes size meets size requirements.
#
# Throws std::invalid_argument on failure.
def validate_universal_hash_bytes(hash_bytes: int, min_hash_bytes: int) -> None: ...
