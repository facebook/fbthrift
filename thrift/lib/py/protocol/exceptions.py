# Copyright (c) Facebook, Inc. and its affiliates.
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


def create_ThriftUnicodeDecodeError_from_UnicodeDecodeError(error, field_name):
    return ThriftUnicodeDecodeError(
        error.encoding, error.object, error.start, error.end, error.reason, field_name
    )


class ThriftUnicodeDecodeError(UnicodeDecodeError):
    def __init__(self, encoding, object, start, end, reason, field_name):
        super(UnicodeDecodeError, self).__init__(encoding, object, start, end, reason)
        self.field_name = field_name

    def __str__(self):
        return "{error} when decoding field '{field}'".format(
            error=super(UnicodeDecodeError, self).__str__(), field=self.field_name
        )
