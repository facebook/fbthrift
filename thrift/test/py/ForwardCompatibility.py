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

import unittest
if False:
    import typing as t  # noqa

from thrift.protocol import TBinaryProtocol, \
    TCompactProtocol, TSimpleJSONProtocol
from thrift.util import Serializer

from ForwardCompatibility.ForwardCompatibility.ttypes import \
    NewStructure, OldStructure, \
    NewStructureNested, OldStructureNested


class AbstractTest(object):
    protocol_factory = None

    def _serialize(self, obj):
        return Serializer.serialize(self.protocol_factory, obj)

    def _deserialize(self, objtype, data):
        return Serializer.deserialize(self.protocol_factory, data, objtype())


def dummy(features):
    # type: (t.Dict[int, float]) -> bool
    return 0 in features


def dummynested(features):
    # type: (t.List[t.Dict[int, float]]) -> bool
    return bool(len(features)) and 0 in features[0]


class TestForwardCompatibilityAbstract(AbstractTest, unittest.TestCase):
    def assertFeaturesAlmostEqual(self, a, b):
        # type: (float, float) -> None
        self.assertTrue(abs(a - b) < 1e-3)

    def testPrimitiveType(self):
        # type: () -> None
        if self.protocol_factory is None:
            self.skipTest("abstract class")
        old = OldStructure()
        old.features = None
        old.features = {}
        old.features[1] = 100.1
        old.features[217] = 314.5
        self.assertFalse(dummy(old.features))
        sOld = self._serialize(old)
        new = self._deserialize(NewStructure, sOld)
        self.assertFeaturesAlmostEqual(new.features[1], 100.1)
        self.assertFeaturesAlmostEqual(new.features[217], 314.5)
        sNew = self._serialize(new)
        new2 = self._deserialize(NewStructure, sNew)
        self.assertFeaturesAlmostEqual(new2.features[1], 100.1)
        self.assertFeaturesAlmostEqual(new2.features[217], 314.5)

    def testNested(self):
        # type: () -> None
        if self.protocol_factory is None:
            self.skipTest("abstract class")
        old = OldStructureNested()
        old.features = None
        old.features = [{}]
        old.features[0][1] = 100.1
        old.features[0][217] = 314.5
        self.assertFalse(dummynested(old.features))
        sOld = self._serialize(old)
        new = self._deserialize(NewStructureNested, sOld)
        self.assertFeaturesAlmostEqual(new.features[0][1], 100.1)
        self.assertFeaturesAlmostEqual(new.features[0][217], 314.5)
        sNew = self._serialize(new)
        new2 = self._deserialize(NewStructureNested, sNew)
        self.assertFeaturesAlmostEqual(new2.features[0][1], 100.1)
        self.assertFeaturesAlmostEqual(new2.features[0][217], 314.5)


class TestForwardCompatibilityBinary(TestForwardCompatibilityAbstract):
    protocol_factory = TBinaryProtocol.TBinaryProtocolFactory()


class TestForwardCompatibilityCompact(TestForwardCompatibilityAbstract):
    protocol_factory = TCompactProtocol.TCompactProtocolFactory()


class TestForwardCompatibilityBinaryAccelerated(TestForwardCompatibilityAbstract):
    protocol_factory = TBinaryProtocol.TBinaryProtocolAcceleratedFactory()


class TestForwardCompatibilityCompactAccelerated(TestForwardCompatibilityAbstract):
    protocol_factory = TCompactProtocol.TCompactProtocolAcceleratedFactory()


class TestForwardCompatibilityJSON(TestForwardCompatibilityAbstract):
    protocol_factory = TSimpleJSONProtocol.TSimpleJSONProtocolFactory()


if __name__ == "__main__":
    unittest.main()
