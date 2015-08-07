from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import itertools
import unittest

import six.moves as sm

from thrift.protocol.THeaderProtocol import THeaderProtocol
from thrift.transport.TTransport import TMemoryBuffer
from thrift.transport.THeaderTransport import THeaderTransport
from thrift.transport.TFuzzyHeaderTransport import TFuzzyHeaderTransport

from fuzz import ttypes

class FuzzyTransportTest(object):
    """Test class that sets up a THeaderTransport and a TFuzzyHeaderTransport.

    Used for writing and comparing  messages using both transports.
    """
    fuzz_fields = []

    # Sample TestService method arguments
    sampleListStruct = ttypes.ListStruct(
        a=[True, False],
        b=[1, 2, 3],
        c=[1.2, 3.4],
        d=["ab", "cd"],
        e=[list(sm.xrange(n)) for n in sm.xrange(20)],
        f=[{1: 2}, {3: 4, 5: 6}],
        g=[{"a", "b"}, {"c"}, set()]
    )

    def setUp(self):
        """Create two buffers, transports, and protocols.

        self._h_trans uses THeaderTransport
        self._f_trans uses TFuzzyHeaderTransport
        """
        cls = self.__class__

        # THeaderTransport attributes
        self._h_buf = TMemoryBuffer()
        self._h_trans = THeaderTransport(self._h_buf)
        self._h_prot = THeaderProtocol(self._h_trans)

        # TFuzzyHeaderTransport attributes
        self._f_buf = TMemoryBuffer()
        self._f_trans = TFuzzyHeaderTransport(
            self._f_buf, fuzz_fields=cls.fuzz_fields,
            fuzz_all_if_empty=False, verbose=False)
        self._f_prot = THeaderProtocol(self._f_trans)

    def writeObject(self, obj=sampleListStruct):
        """Write an object to the test and reference protocols.

        Return the contents of both buffers.
        """
        obj.write(self._h_prot)
        obj.write(self._f_prot)

        self._h_trans.flush()
        self._f_trans.flush()

        h_buf = self._h_buf.getvalue()
        f_buf = self._f_buf.getvalue()

        return h_buf, f_buf

    def differentIndices(self, header, fuzzy):
        """Return a list of byte positions at which two messages' bytes differ.

        Header should be the contents of self._h_buf
        Fuzzy should be the contents of self._f_buf
        """
        indices = []
        for i, (h, f) in enumerate(itertools.izip(header, fuzzy)):
            if h != f:
                indices.append(i)
        return indices

    def assertEqualsExceptIndices(self, header, fuzzy, indices):
        """Assert that the buffers `header` and `fuzzy` are equal,
        except possibly at the byte positions included in `indices`.

        This ensures that the message produced by TFuzzyHeaderProtocol (fuzzy)
        is equal to the message produced by THeaderProtocol (header), except
        at the byte positions that are expected to be fuzzed."""
        self.assertEquals(len(header), len(fuzzy))
        for diff in self.differentIndices(header, fuzzy):
            self.assertIn(diff, indices)

class NoFuzzTest(FuzzyTransportTest, unittest.TestCase):
    """Check that the serialized messages are equal when no transport
    fuzzing is done"""
    def testEqual(self):
        header, fuzzy = self.writeObject()
        self.assertEquals(header, fuzzy)

class FuzzLengthTest(FuzzyTransportTest, unittest.TestCase):
    fuzz_fields = ["length"]

    def testLengthFuzzed(self):
        header, fuzzy = self.writeObject()

        # The first 4 bytes may be different but everything
        # else should remain equal
        expected_different = {0, 1, 2, 3}
        self.assertEqualsExceptIndices(header, fuzzy, expected_different)

class FuzzMagicTest(FuzzyTransportTest, unittest.TestCase):
    fuzz_fields = ["magic"]

    def testMagicFuzzed(self):
        header, fuzzy = self.writeObject()

        # The magic bytes (4, 5) may be different
        expected_different = {4, 5}
        self.assertEqualsExceptIndices(header, fuzzy, expected_different)

class FuzzFlagsTest(FuzzyTransportTest, unittest.TestCase):
    fuzz_fields = ["flags"]

    def testFlagsFuzzed(self):
        header, fuzzy = self.writeObject()

        # The flags bytes (6, 7) may be different
        expected_different = {6, 7}
        self.assertEqualsExceptIndices(header, fuzzy, expected_different)

class FuzzSeqIdTest(FuzzyTransportTest, unittest.TestCase):
    fuzz_fields = ["seq_id"]

    def testSeqIdFuzzed(self):
        header, fuzzy = self.writeObject()

        # The seq_id bytes (8 - 11) may be different
        expected_different = {8, 9, 10, 11}
        self.assertEqualsExceptIndices(header, fuzzy, expected_different)

class FuzzHeaderSizeTest(FuzzyTransportTest, unittest.TestCase):
    fuzz_fields = ["header_size"]

    def testHeaderSizeFuzzed(self):
        header, fuzzy = self.writeObject()

        # The header_size bytes (12, 13) may be different
        expected_different = {12, 13}
        self.assertEqualsExceptIndices(header, fuzzy, expected_different)

if __name__ == '__main__':
    unittest.main()
