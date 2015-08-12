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

import unittest

class TestPythonConstants(unittest.TestCase):
    def testStrings(self):
        from .constants import constants
        self.assertEquals(constants.apostrophe, "'")
        self.assertEquals(constants.tripleApostrophe, "'''")
        self.assertEquals(constants.quotationMark, '"')
        self.assertEquals(constants.backslash, "\\")

    def testDict(self):
        from .constants import constants
        self.assertEquals(constants.escapeChars['apostrophe'], "'")
        self.assertEquals(constants.escapeChars['quotationMark'], '"')
        self.assertEquals(constants.escapeChars['backslash'], "\\")

if __name__ == '__main__':
    unittest.main()
