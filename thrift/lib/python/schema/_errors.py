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


"""The codec's error types, re-exported by ``serialization``. They live here
so the protocol backends can raise them without importing the walk."""


class DecodeError(ValueError):
    """Raised when input is malformed, truncated, too deeply nested, or does
    not match the ``TypeRef`` it is decoded against."""


class EncodeError(ValueError):
    """Raised when a value does not fit the ``TypeRef`` it is encoded as."""
