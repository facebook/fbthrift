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

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import trollius as asyncio


def call_as_future(callable, loop, *args, **kwargs):
    """This is a copy of thrift.util.asyncio. So, let's consider unifying them.

        call_as_future(callable, *args, **kwargs) -> trollius.Task

    Like trollius.ensure_future() but takes any callable and converts
    it to a coroutine function first.
    """
    if not asyncio.iscoroutinefunction(callable):
        callable = asyncio.coroutine(callable)

    return asyncio.ensure_future(callable(*args, **kwargs), loop=loop)
