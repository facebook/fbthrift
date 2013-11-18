#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements. See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership. The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License. You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the License for the
# specific language governing permissions and limitations
# under the License.
#

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import sys

if sys.version_info[0] >= 3:

    from io import BytesIO

    class BytesStrIO(BytesIO):
        def __init__(self, *args):
            args_new = []
            for arg in args:
                if not isinstance(arg, bytes):
                    args_new.append(arg.encode())
                else:
                    args_new.append(arg)
            BytesIO.__init__(self, *args_new)

        def write(self, data):
            if isinstance(data, bytes):
                BytesIO.write(self, data)
            else:
                BytesIO.write(self, data.encode())
