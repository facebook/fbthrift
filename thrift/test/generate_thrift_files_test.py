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

import filecmp
import os
import tempfile
import unittest
from subprocess import check_call


EXEC = os.getenv("GENERATE_THRIFT_FILES_EXECUTABLE")
DIR = os.getenv("GENERATE_THRIFT_FILES_DIRECTORY")
ERROR_MESSAGE = """\
One or more thrift files are out of sync. \
To sync them, run: `buck run //thrift/test:generate_thrift_files`
"""


class Test(unittest.TestCase):
    def compare_two_directories(self, dcmp: filecmp.dircmp):
        self.assertFalse(dcmp.left_only, msg=ERROR_MESSAGE)
        self.assertFalse(dcmp.right_only, msg=ERROR_MESSAGE)
        self.assertFalse(dcmp.diff_files, msg=ERROR_MESSAGE)
        for sub_dcmp in dcmp.subdirs.values():
            self.compare_two_directories(sub_dcmp)

    def test_compare(self):
        with tempfile.TemporaryDirectory() as tmp:
            check_call([EXEC, "--install_dir", os.path.join(tmp, "gen_if")])
            dcmp = filecmp.dircmp(DIR, tmp)
            dcmp.report_full_closure()
            self.compare_two_directories(dcmp)
