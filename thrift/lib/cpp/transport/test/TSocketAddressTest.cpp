/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#include "thrift/lib/cpp/transport/TSocketAddress.h"
#include "thrift/lib/cpp/transport/TTransportException.h"

#include <boost/test/unit_test.hpp>
#include <iostream>
#include <sstream>

using namespace boost;
using std::string;
using std::cerr;
using std::endl;
using apache::thrift::transport::TSocketAddress;
using apache::thrift::transport::TTransportException;

BOOST_AUTO_TEST_CASE(ConstructFromIpv4) {
  TSocketAddress addr("1.2.3.4", 4321);
  BOOST_CHECK_EQUAL(addr.getFamily(), AF_INET);
  BOOST_CHECK_EQUAL(addr.getAddressStr(), "1.2.3.4");
  BOOST_CHECK_EQUAL(addr.getPort(), 4321);
  const struct sockaddr_in* inaddr =
    reinterpret_cast<const sockaddr_in*>(addr.getAddress());
  BOOST_CHECK_EQUAL(inaddr->sin_addr.s_addr, htonl(0x01020304));
  BOOST_CHECK_EQUAL(inaddr->sin_port, htons(4321));
}

BOOST_AUTO_TEST_CASE(IPv4ToStringConversion) {
  // testing addresses *.5.5.5, 5.*.5.5, 5.5.*.5, 5.5.5.*
  TSocketAddress addr;
  for (int pos = 0; pos < 4; ++pos) {
    for (int i = 0; i < 256; ++i) {
      int fragments[] = {5,5,5,5};
      fragments[pos] = i;
      std::ostringstream ss;
      ss << fragments[0] << "." << fragments[1] << "."
         << fragments[2] << "." << fragments[3];
      string ipString = ss.str();
      addr.setFromIpPort(ipString, 1234);
      BOOST_CHECK_EQUAL(addr.getAddressStr(), ipString);
    }
  }
}

BOOST_AUTO_TEST_CASE(SetFromIpv4) {
  TSocketAddress addr;
  addr.setFromIpPort("255.254.253.252", 8888);
  BOOST_CHECK_EQUAL(addr.getFamily(), AF_INET);
  BOOST_CHECK_EQUAL(addr.getAddressStr(), "255.254.253.252");
  BOOST_CHECK_EQUAL(addr.getPort(), 8888);
  const struct sockaddr_in* inaddr =
    reinterpret_cast<const sockaddr_in*>(addr.getAddress());
  BOOST_CHECK_EQUAL(inaddr->sin_addr.s_addr, htonl(0xfffefdfc));
  BOOST_CHECK_EQUAL(inaddr->sin_port, htons(8888));
}

BOOST_AUTO_TEST_CASE(ConstructFromInvalidIpv4) {
  BOOST_CHECK_THROW(TSocketAddress("1.2.3.256", 1234), TTransportException);
}

BOOST_AUTO_TEST_CASE(SetFromInvalidIpv4) {
  TSocketAddress addr("12.34.56.78", 80);

  // Try setting to an invalid value
  // Since setFromIpPort() shouldn't allow hostname lookups, setting to
  // "localhost" should fail, even if localhost is resolvable
  BOOST_CHECK_THROW(addr.setFromIpPort("localhost", 1234),
                    TTransportException);

  // Make sure the address still has the old contents
  BOOST_CHECK_EQUAL(addr.getFamily(), AF_INET);
  BOOST_CHECK_EQUAL(addr.getAddressStr(), "12.34.56.78");
  BOOST_CHECK_EQUAL(addr.getPort(), 80);
  const struct sockaddr_in* inaddr =
    reinterpret_cast<const sockaddr_in*>(addr.getAddress());
  BOOST_CHECK_EQUAL(inaddr->sin_addr.s_addr, htonl(0x0c22384e));
}

BOOST_AUTO_TEST_CASE(SetFromHostname) {
  // hopefully "localhost" is resolvable on any system that will run the tests
  BOOST_CHECK_THROW(TSocketAddress("localhost", 80), TTransportException);
  TSocketAddress addr("localhost", 80, true);

  TSocketAddress addr2;
  BOOST_CHECK_THROW(addr2.setFromIpPort("localhost", 0), TTransportException);
  addr2.setFromHostPort("localhost", 0);
}

BOOST_AUTO_TEST_CASE(SetFromStrings) {
  TSocketAddress addr;

  // Set from a numeric port string
  addr.setFromLocalPort("1234");
  BOOST_CHECK_EQUAL(addr.getPort(), 1234);

  // setFromLocalPort() should not accept service names
  BOOST_CHECK_THROW(addr.setFromLocalPort("http"), TTransportException);

  // Call setFromLocalIpPort() with just a port, no IP
  addr.setFromLocalIpPort("80");
  BOOST_CHECK_EQUAL(addr.getPort(), 80);

  // Call setFromLocalIpPort() with an IP and port.
  addr.setFromLocalIpPort("127.0.0.1:4321");
  BOOST_CHECK_EQUAL(addr.getAddressStr(), "127.0.0.1");
  BOOST_CHECK_EQUAL(addr.getPort(), 4321);

  // setFromIpPort() without an address should fail
  BOOST_CHECK_THROW(addr.setFromIpPort("4321"), TTransportException);

  // Call setFromIpPort() with an IPv6 address and port
  addr.setFromIpPort("2620:0:1cfe:face:b00c::3:65535");
  BOOST_CHECK_EQUAL(addr.getFamily(), AF_INET6);
  BOOST_CHECK_EQUAL(addr.getAddressStr(), "2620:0:1cfe:face:b00c::3");
  BOOST_CHECK_EQUAL(addr.getPort(), 65535);

  // Call setFromIpPort() with an IPv4 address and port
  addr.setFromIpPort("1.2.3.4:9999");
  BOOST_CHECK_EQUAL(addr.getFamily(), AF_INET);
  BOOST_CHECK_EQUAL(addr.getAddressStr(), "1.2.3.4");
  BOOST_CHECK_EQUAL(addr.getPort(), 9999);
}

BOOST_AUTO_TEST_CASE(EqualityAndHash) {
  // IPv4
  TSocketAddress local1("127.0.0.1", 1234);
  BOOST_CHECK_EQUAL(local1, local1);
  BOOST_CHECK_EQUAL(local1.hash(), local1.hash());

  TSocketAddress local2("127.0.0.1", 1234);
  BOOST_CHECK_EQUAL(local1, local2);
  BOOST_CHECK_EQUAL(local1.hash(), local2.hash());

  TSocketAddress local3("127.0.0.1", 4321);
  BOOST_CHECK_NE(local1, local3);
  BOOST_CHECK_NE(local1.hash(), local3.hash());

  TSocketAddress other1("1.2.3.4", 1234);
  BOOST_CHECK_EQUAL(other1, other1);
  BOOST_CHECK_EQUAL(other1.hash(), other1.hash());
  BOOST_CHECK_NE(local1, other1);
  BOOST_CHECK_NE(local1.hash(), other1.hash());

  TSocketAddress other2("4.3.2.1", 1234);
  BOOST_CHECK_NE(other1.hash(), other2.hash());
  BOOST_CHECK_NE(other1.hash(), other2.hash());

  other2.setFromIpPort("1.2.3.4", 0);
  BOOST_CHECK_NE(other1.hash(), other2.hash());
  BOOST_CHECK_NE(other1.hash(), other2.hash());
  other2.setPort(1234);
  BOOST_CHECK_EQUAL(other1.hash(), other2.hash());
  BOOST_CHECK_EQUAL(other1.hash(), other2.hash());

  // IPv6
  TSocketAddress v6_1("2620:0:1c00:face:b00c:0:0:abcd", 1234);
  TSocketAddress v6_2("2620:0:1c00:face:b00c::abcd", 1234);
  TSocketAddress v6_3("2620:0:1c00:face:b00c::bcda", 1234);
  BOOST_CHECK_EQUAL(v6_1, v6_2);
  BOOST_CHECK_EQUAL(v6_1.hash(), v6_2.hash());
  BOOST_CHECK_NE(v6_1, v6_3);
  BOOST_CHECK_NE(v6_1.hash(), v6_3.hash());

  // IPv4 versus IPv6 comparison
  TSocketAddress localIPv6("::1", 1234);
  // Even though these both refer to localhost,
  // IPv4 and IPv6 addresses are never treated as the same address
  BOOST_CHECK_NE(local1, localIPv6);
  BOOST_CHECK_NE(local1.hash(), localIPv6.hash());

  // IPv4-mapped IPv6 addresses are not treated as equal
  // to the equivalent IPv4 address
  TSocketAddress v4("10.0.0.3", 99);
  TSocketAddress v6_mapped1("::ffff:10.0.0.3", 99);
  TSocketAddress v6_mapped2("::ffff:0a00:0003", 99);
  BOOST_CHECK_NE(v4, v6_mapped1);
  BOOST_CHECK_NE(v4, v6_mapped2);
  BOOST_CHECK_EQUAL(v6_mapped1, v6_mapped2);

  // However, after calling convertToIPv4(), the mapped address should now be
  // equal to the v4 version.
  BOOST_CHECK(v6_mapped1.isIPv4Mapped());
  v6_mapped1.convertToIPv4();
  BOOST_CHECK_EQUAL(v6_mapped1, v4);
  BOOST_CHECK_NE(v6_mapped1, v6_mapped2);

  // Unix
  TSocketAddress unix1;
  unix1.setFromPath("/foo");
  TSocketAddress unix2;
  unix2.setFromPath("/foo");
  TSocketAddress unix3;
  unix3.setFromPath("/bar");
  TSocketAddress unixAnon;
  unixAnon.setFromPath("");

  BOOST_CHECK_EQUAL(unix1, unix2);
  BOOST_CHECK_EQUAL(unix1.hash(), unix2.hash());
  BOOST_CHECK_NE(unix1, unix3);
  BOOST_CHECK_NE(unix1, unixAnon);
  BOOST_CHECK_NE(unix2, unix3);
  BOOST_CHECK_NE(unix2, unixAnon);
  // anonymous addresses aren't equal to any other address,
  // including themselves
  BOOST_CHECK_NE(unixAnon, unixAnon);

  // It isn't strictly required that hashes for different addresses be
  // different, but we should have very few collisions.  It generally indicates
  // a problem if these collide
  BOOST_CHECK_NE(unix1.hash(), unix3.hash());
  BOOST_CHECK_NE(unix1.hash(), unixAnon.hash());
  BOOST_CHECK_NE(unix3.hash(), unixAnon.hash());
}

BOOST_AUTO_TEST_CASE(IsPrivate) {
  // IPv4
  TSocketAddress addr("9.255.255.255", 0);
  BOOST_CHECK(!addr.isPrivateAddress());
  addr.setFromIpPort("10.0.0.0", 0);
  BOOST_CHECK(addr.isPrivateAddress());
  addr.setFromIpPort("10.255.255.255", 0);
  BOOST_CHECK(addr.isPrivateAddress());
  addr.setFromIpPort("11.0.0.0", 0);
  BOOST_CHECK(!addr.isPrivateAddress());

  addr.setFromIpPort("172.15.255.255", 0);
  BOOST_CHECK(!addr.isPrivateAddress());
  addr.setFromIpPort("172.16.0.0", 0);
  BOOST_CHECK(addr.isPrivateAddress());
  addr.setFromIpPort("172.31.255.255", 0);
  BOOST_CHECK(addr.isPrivateAddress());
  addr.setFromIpPort("172.32.0.0", 0);
  BOOST_CHECK(!addr.isPrivateAddress());

  addr.setFromIpPort("192.167.255.255", 0);
  BOOST_CHECK(!addr.isPrivateAddress());
  addr.setFromIpPort("192.168.0.0", 0);
  BOOST_CHECK(addr.isPrivateAddress());
  addr.setFromIpPort("192.168.255.255", 0);
  BOOST_CHECK(addr.isPrivateAddress());
  addr.setFromIpPort("192.169.0.0", 0);
  BOOST_CHECK(!addr.isPrivateAddress());

  addr.setFromIpPort("126.255.255.255", 0);
  BOOST_CHECK(!addr.isPrivateAddress());
  addr.setFromIpPort("127.0.0.0", 0);
  BOOST_CHECK(addr.isPrivateAddress());
  addr.setFromIpPort("127.0.0.1", 0);
  BOOST_CHECK(addr.isPrivateAddress());
  addr.setFromIpPort("127.255.255.255", 0);
  BOOST_CHECK(addr.isPrivateAddress());
  addr.setFromIpPort("128.0.0.0", 0);
  BOOST_CHECK(!addr.isPrivateAddress());

  addr.setFromIpPort("1.2.3.4", 0);
  BOOST_CHECK(!addr.isPrivateAddress());
  addr.setFromIpPort("69.171.239.10", 0);
  BOOST_CHECK(!addr.isPrivateAddress());

  // IPv6
  addr.setFromIpPort("fbff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", 0);
  BOOST_CHECK(!addr.isPrivateAddress());
  addr.setFromIpPort("fc00::", 0);
  BOOST_CHECK(addr.isPrivateAddress());
  addr.setFromIpPort("fdff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", 0);
  BOOST_CHECK(addr.isPrivateAddress());
  addr.setFromIpPort("fe00::", 0);
  BOOST_CHECK(!addr.isPrivateAddress());

  addr.setFromIpPort("fe7f:ffff:ffff:ffff:ffff:ffff:ffff:ffff", 0);
  BOOST_CHECK(!addr.isPrivateAddress());
  addr.setFromIpPort("fe80::", 0);
  BOOST_CHECK(addr.isPrivateAddress());
  addr.setFromIpPort("fe80::ffff:ffff:ffff:ffff", 0);
  BOOST_CHECK(addr.isPrivateAddress());
  addr.setFromIpPort("febf:ffff:ffff:ffff:ffff:ffff:ffff:ffff", 0);
  BOOST_CHECK(addr.isPrivateAddress());
  addr.setFromIpPort("fec0::", 0);
  BOOST_CHECK(!addr.isPrivateAddress());

  addr.setFromIpPort("::0", 0);
  BOOST_CHECK(!addr.isPrivateAddress());
  addr.setFromIpPort("2620:0:1c00:face:b00c:0:0:abcd", 0);
  BOOST_CHECK(!addr.isPrivateAddress());

  // IPv4-mapped IPv6
  addr.setFromIpPort("::ffff:127.0.0.1", 0);
  BOOST_CHECK(addr.isPrivateAddress());
  addr.setFromIpPort("::ffff:10.1.2.3", 0);
  BOOST_CHECK(addr.isPrivateAddress());
  addr.setFromIpPort("::ffff:172.24.0.115", 0);
  BOOST_CHECK(addr.isPrivateAddress());
  addr.setFromIpPort("::ffff:192.168.0.1", 0);
  BOOST_CHECK(addr.isPrivateAddress());
  addr.setFromIpPort("::ffff:69.171.239.10", 0);
  BOOST_CHECK(!addr.isPrivateAddress());

  // Unix sockets are considered private addresses
  addr.setFromPath("/tmp/mysock");
  BOOST_CHECK(addr.isPrivateAddress());
}

BOOST_AUTO_TEST_CASE(IsLoopback) {
  // IPv4
  TSocketAddress addr("127.0.0.1", 0);
  BOOST_CHECK(addr.isLoopbackAddress());
  addr.setFromIpPort("127.0.0.0", 0xffff);
  BOOST_CHECK(addr.isLoopbackAddress());
  addr.setFromIpPort("127.1.1.1", 0xffff);
  BOOST_CHECK(addr.isLoopbackAddress());
  addr.setFromIpPort("127.255.255.255", 80);
  BOOST_CHECK(addr.isLoopbackAddress());

  addr.setFromIpPort("128.0.0.0", 0);
  BOOST_CHECK(!addr.isLoopbackAddress());
  addr.setFromIpPort("126.255.255.255", 0);
  BOOST_CHECK(!addr.isLoopbackAddress());
  addr.setFromIpPort("10.1.2.3", 0);
  BOOST_CHECK(!addr.isLoopbackAddress());

  // IPv6
  addr.setFromIpPort("::1", 0);
  BOOST_CHECK(addr.isLoopbackAddress());
  addr.setFromIpPort("::0", 0);
  BOOST_CHECK(!addr.isLoopbackAddress());
  addr.setFromIpPort("::2", 0);
  BOOST_CHECK(!addr.isLoopbackAddress());

  // IPv4-mapped IPv6
  addr.setFromIpPort("::ffff:127.0.0.1", 0);
  BOOST_CHECK(addr.isLoopbackAddress());
  addr.setFromIpPort("::ffff:7f0a:141e", 0);
  BOOST_CHECK(addr.isLoopbackAddress());
  addr.setFromIpPort("::ffff:169.254.0.13", 0);
  BOOST_CHECK(!addr.isLoopbackAddress());

  // Unix sockets are considered loopback addresses
  addr.setFromPath("/tmp/mysock");
  BOOST_CHECK(addr.isLoopbackAddress());
}

void CheckPrefixMatch(const TSocketAddress& first,
    const TSocketAddress& second, unsigned matchingPrefixLen) {
  unsigned i;
  for (i = 0; i <= matchingPrefixLen; i++) {
    BOOST_CHECK(first.prefixMatch(second, i));
  }
  unsigned addrLen = (first.getFamily() == AF_INET6) ? 128 : 32;
  for (; i <= addrLen; i++) {
    BOOST_CHECK(!first.prefixMatch(second, i));
  }
}

BOOST_AUTO_TEST_CASE(PrefixMatch) {
  // IPv4
  TSocketAddress addr1("127.0.0.1", 0);
  TSocketAddress addr2("127.0.0.1", 0);
  CheckPrefixMatch(addr1, addr2, 32);

  addr2.setFromIpPort("127.0.1.1", 0);
  CheckPrefixMatch(addr1, addr2, 23);

  addr2.setFromIpPort("1.1.0.127", 0);
  CheckPrefixMatch(addr1, addr2, 1);

  // Address family mismatch
  addr2.setFromIpPort("::ffff:127.0.0.1", 0);
  BOOST_CHECK(!addr1.prefixMatch(addr2, 1));

  // IPv6
  addr1.setFromIpPort("2a03:2880:10:8f02:face:b00c:0:25", 0);
  CheckPrefixMatch(addr1, addr2, 2);

  addr2.setFromIpPort("2a03:2880:10:8f02:face:b00c:0:25", 0);
  CheckPrefixMatch(addr1, addr2, 128);

  addr2.setFromIpPort("2a03:2880:30:8f02:face:b00c:0:25", 0);
  CheckPrefixMatch(addr1, addr2, 42);
}

void CheckFirstLessThanSecond(TSocketAddress first, TSocketAddress second) {
  BOOST_CHECK(!(first < first));
  BOOST_CHECK(!(second < second));
  BOOST_CHECK(first < second);
  BOOST_CHECK(!(first == second));
  BOOST_CHECK(!(second < first));
}

BOOST_AUTO_TEST_CASE(CheckComparatorBehavior) {
  TSocketAddress first, second;

  // The following comparison are strict (so if first and second were
  // inverted that is ok.

  //IP V4

  // port comparisions
  first.setFromIpPort("128.0.0.0", 0);
  second.setFromIpPort("128.0.0.0", 0xFFFF);
  CheckFirstLessThanSecond(first, second);
  first.setFromIpPort("128.0.0.100", 0);
  second.setFromIpPort("128.0.0.0", 0xFFFF);
  CheckFirstLessThanSecond(first, second);

  // Address comparisons
  first.setFromIpPort("128.0.0.0", 10);
  second.setFromIpPort("128.0.0.100", 10);
  CheckFirstLessThanSecond(first, second);

  // Comaprision between IPV4 and IPV6
  first.setFromIpPort("128.0.0.0", 0);
  second.setFromIpPort("::ffff:127.0.0.1", 0);
  CheckFirstLessThanSecond(first, second);
  first.setFromIpPort("128.0.0.0", 100);
  second.setFromIpPort("::ffff:127.0.0.1", 0);
  CheckFirstLessThanSecond(first, second);

  // IPV6 comparisons

  // port comparisions
  first.setFromIpPort("::0", 0);
  second.setFromIpPort("::0", 0xFFFF);
  CheckFirstLessThanSecond(first, second);
  first.setFromIpPort("::0", 0);
  second.setFromIpPort("::1", 0xFFFF);
  CheckFirstLessThanSecond(first, second);

  // Address comparisons
  first.setFromIpPort("::0", 10);
  second.setFromIpPort("::1", 10);
  CheckFirstLessThanSecond(first, second);

  // Unix
  first.setFromPath("/foo");
  second.setFromPath("/1234");
  // The exact comparison order doesn't really matter, as long as
  // (a < b), (b < a), and (a == b) are consistent.
  // This checks our current comparison rules, which checks the path length
  // before the path contents.
  CheckFirstLessThanSecond(first, second);
  first.setFromPath("/1234");
  second.setFromPath("/5678");
  CheckFirstLessThanSecond(first, second);

  // IPv4 vs Unix.
  // We currently compare the address family values, and AF_UNIX < AF_INET
  first.setFromPath("/foo");
  second.setFromIpPort("127.0.0.1", 80);
  CheckFirstLessThanSecond(first, second);
}

BOOST_AUTO_TEST_CASE(Unix) {
  TSocketAddress addr;

  // Test a small path
  addr.setFromPath("foo");
  BOOST_CHECK_EQUAL(addr.getFamily(), AF_UNIX);
  BOOST_CHECK_EQUAL(addr.describe(), "foo");
  BOOST_CHECK_THROW(addr.getAddressStr(), TTransportException);
  BOOST_CHECK_THROW(addr.getPort(), TTransportException);
  BOOST_CHECK(addr.isPrivateAddress());
  BOOST_CHECK(addr.isLoopbackAddress());

  // Test a path that is too large
  const char longPath[] =
    "abcdefghijklmnopqrstuvwxyz0123456789"
    "abcdefghijklmnopqrstuvwxyz0123456789"
    "abcdefghijklmnopqrstuvwxyz0123456789"
    "abcdefghijklmnopqrstuvwxyz0123456789";
  BOOST_CHECK_THROW(addr.setFromPath(longPath), TTransportException);
  // The original address should still be the same
  BOOST_CHECK_EQUAL(addr.getFamily(), AF_UNIX);
  BOOST_CHECK_EQUAL(addr.describe(), "foo");

  // Test a path that exactly fits in sockaddr_un
  // (not including the NUL terminator)
  const char exactLengthPath[] =
    "abcdefghijklmnopqrstuvwxyz0123456789"
    "abcdefghijklmnopqrstuvwxyz0123456789"
    "abcdefghijklmnopqrstuvwxyz0123456789";
  addr.setFromPath(exactLengthPath);
  BOOST_CHECK_EQUAL(addr.describe(), exactLengthPath);

  // Test converting a unix socket address to an IPv4 one, then back
  addr.setFromHostPort("127.0.0.1", 1234);
  BOOST_CHECK_EQUAL(addr.getFamily(), AF_INET);
  BOOST_CHECK_EQUAL(addr.describe(), "127.0.0.1:1234");
  addr.setFromPath("/i/am/a/unix/address");
  BOOST_CHECK_EQUAL(addr.getFamily(), AF_UNIX);
  BOOST_CHECK_EQUAL(addr.describe(), "/i/am/a/unix/address");

  // Test copy constructor and assignment operator
  {
    TSocketAddress copy(addr);
    BOOST_CHECK_EQUAL(copy, addr);
    copy.setFromPath("/abc");
    BOOST_CHECK_NE(copy, addr);
    copy = addr;
    BOOST_CHECK_EQUAL(copy, addr);
    copy.setFromIpPort("127.0.0.1", 80);
    BOOST_CHECK_NE(copy, addr);
    copy = addr;
    BOOST_CHECK_EQUAL(copy, addr);
  }

  {
    TSocketAddress copy(addr);
    BOOST_CHECK_EQUAL(copy, addr);
    BOOST_CHECK_EQUAL(copy.describe(), "/i/am/a/unix/address");
    BOOST_CHECK_EQUAL(copy.getPath(), "/i/am/a/unix/address");

    TSocketAddress other("127.0.0.1", 80);
    BOOST_CHECK_NE(other, addr);
    other = copy;
    BOOST_CHECK_EQUAL(other, copy);
    BOOST_CHECK_EQUAL(other, addr);
    BOOST_CHECK_EQUAL(copy, addr);
  }

#if __GXX_EXPERIMENTAL_CXX0X__
  {
    TSocketAddress copy;
    {
      // move a unix address into a non-unix address
      TSocketAddress tmpCopy(addr);
      copy = std::move(tmpCopy);
    }
    BOOST_CHECK_EQUAL(copy, addr);

    copy.setFromPath("/another/path");
    {
      // move a unix address into a unix address
      TSocketAddress tmpCopy(addr);
      copy = std::move(tmpCopy);
    }
    BOOST_CHECK_EQUAL(copy, addr);

    {
      // move a non-unix address into a unix address
      TSocketAddress tmp("127.0.0.1", 80);
      copy = std::move(tmp);
    }
    BOOST_CHECK_EQUAL(copy.getAddressStr(), "127.0.0.1");
    BOOST_CHECK_EQUAL(copy.getPort(), 80);

    copy = addr;
    // move construct a unix address
    TSocketAddress other(std::move(copy));
    BOOST_CHECK_EQUAL(other, addr);
    BOOST_CHECK_EQUAL(other.getPath(), addr.getPath());
  }
#endif
}

BOOST_AUTO_TEST_CASE(AnonymousUnix) {
  // Create a unix socket pair, and get the addresses.
  int fds[2];
  int rc = socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
  BOOST_REQUIRE_EQUAL(rc, 0);

  TSocketAddress addr0;
  TSocketAddress peer0;
  TSocketAddress addr1;
  TSocketAddress peer1;
  addr0.setFromLocalAddress(fds[0]);
  peer0.setFromPeerAddress(fds[0]);
  addr1.setFromLocalAddress(fds[1]);
  peer1.setFromPeerAddress(fds[1]);
  close(fds[0]);
  close(fds[1]);

  BOOST_CHECK_EQUAL(addr0.describe(), "<anonymous unix address>");
  BOOST_CHECK_EQUAL(addr1.describe(), "<anonymous unix address>");
  BOOST_CHECK_EQUAL(peer0.describe(), "<anonymous unix address>");
  BOOST_CHECK_EQUAL(peer1.describe(), "<anonymous unix address>");

  // Anonymous addresses should never compare equal
  BOOST_CHECK_NE(addr0, addr1);
  BOOST_CHECK_NE(peer0, peer1);

  // Note that logically addr0 and peer1 are the same,
  // but since they are both anonymous we have no way to determine this
  BOOST_CHECK_NE(addr0, peer1);
  // We can't even tell if an anonymous address is equal to itself
  BOOST_CHECK_NE(addr0, addr0);
}

#define REQUIRE_ERRNO(cond, msg) \
  if (!(cond)) { \
    int _requireErrnoCopy_ = errno; \
    std::ostringstream _requireMsg_; \
    _requireMsg_ << (msg) << ": " << strerror(_requireErrnoCopy_); \
    BOOST_FAIL(_requireMsg_.str()); \
  }

void testSetFromSocket(const TSocketAddress *serverBindAddr,
                       const TSocketAddress *clientBindAddr,
                       TSocketAddress *listenAddrRet,
                       TSocketAddress *acceptAddrRet,
                       TSocketAddress *serverAddrRet,
                       TSocketAddress *serverPeerAddrRet,
                       TSocketAddress *clientAddrRet,
                       TSocketAddress *clientPeerAddrRet) {
  int listenSock = socket(serverBindAddr->getFamily(), SOCK_STREAM, 0);
  REQUIRE_ERRNO(listenSock > 0, "failed to create listen socket");
  const struct sockaddr* laddr = serverBindAddr->getAddress();
  socklen_t laddrLen = serverBindAddr->getActualSize();
  int rc = bind(listenSock, laddr, laddrLen);
  REQUIRE_ERRNO(rc == 0, "failed to bind to server socket");
  rc = listen(listenSock, 10);
  REQUIRE_ERRNO(rc == 0, "failed to listen");

  listenAddrRet->setFromLocalAddress(listenSock);

  TSocketAddress listenPeerAddr;
  BOOST_CHECK_THROW(listenPeerAddr.setFromPeerAddress(listenSock),
                    TTransportException);

  // Note that we use the family from serverBindAddr here, since we allow
  // clientBindAddr to be nullptr.
  int clientSock = socket(serverBindAddr->getFamily(), SOCK_STREAM, 0);
  REQUIRE_ERRNO(clientSock > 0, "failed to create client socket");
  if (clientBindAddr != nullptr) {
    rc = bind(clientSock, clientBindAddr->getAddress(),
              clientBindAddr->getActualSize());
    REQUIRE_ERRNO(rc == 0, "failed to bind to client socket");
  }

  rc = connect(clientSock, listenAddrRet->getAddress(),
               listenAddrRet->getActualSize());
  REQUIRE_ERRNO(rc == 0, "failed to connect");

  socklen_t acceptAddrLen;
  struct sockaddr* acceptAddrStruct = acceptAddrRet->getMutableAddress(
      listenAddrRet->getFamily(), &acceptAddrLen);
  int serverSock = accept(listenSock, acceptAddrStruct, &acceptAddrLen);
  REQUIRE_ERRNO(serverSock > 0, "failed to accept");
  acceptAddrRet->addressUpdated(listenAddrRet->getFamily(), acceptAddrLen);

  serverAddrRet->setFromLocalAddress(serverSock);
  serverPeerAddrRet->setFromPeerAddress(serverSock);
  clientAddrRet->setFromLocalAddress(clientSock);
  clientPeerAddrRet->setFromPeerAddress(clientSock);

  close(clientSock);
  close(serverSock);
  close(listenSock);
}

BOOST_AUTO_TEST_CASE(SetFromSocketIPv4) {
  TSocketAddress serverBindAddr("0.0.0.0", 0);
  TSocketAddress clientBindAddr("0.0.0.0", 0);
  TSocketAddress listenAddr;
  TSocketAddress acceptAddr;
  TSocketAddress serverAddr;
  TSocketAddress serverPeerAddr;
  TSocketAddress clientAddr;
  TSocketAddress clientPeerAddr;

  testSetFromSocket(&serverBindAddr, &clientBindAddr,
                    &listenAddr, &acceptAddr,
                    &serverAddr, &serverPeerAddr,
                    &clientAddr, &clientPeerAddr);

  // The server socket's local address should have the same port as the listen
  // address.  The IP will be different, since the listening socket is
  // listening on INADDR_ANY, but the server socket will have a concrete IP
  // address assigned to it.
  BOOST_CHECK_EQUAL(serverAddr.getPort(), listenAddr.getPort());

  // The client's peer address should always be the same as the server
  // socket's address.
  BOOST_CHECK_EQUAL(clientPeerAddr, serverAddr);
  // The address returned by getpeername() on the server socket should
  // be the same as the address returned by accept()
  BOOST_CHECK_EQUAL(serverPeerAddr, acceptAddr);
  BOOST_CHECK_EQUAL(serverPeerAddr, clientAddr);
  BOOST_CHECK_EQUAL(acceptAddr, clientAddr);
}

/*
 * Note this test exercises Linux-specific Unix socket behavior
 */
BOOST_AUTO_TEST_CASE(SetFromSocketUnixAbstract) {
  // Explicitly binding to an empty path results in an abstract socket
  // name being picked for us automatically.
  TSocketAddress serverBindAddr;
  string path(1, 0);
  path.append("test address");
  serverBindAddr.setFromPath(path);
  TSocketAddress clientBindAddr;
  clientBindAddr.setFromPath("");

  TSocketAddress listenAddr;
  TSocketAddress acceptAddr;
  TSocketAddress serverAddr;
  TSocketAddress serverPeerAddr;
  TSocketAddress clientAddr;
  TSocketAddress clientPeerAddr;

  testSetFromSocket(&serverBindAddr, &clientBindAddr,
                    &listenAddr, &acceptAddr,
                    &serverAddr, &serverPeerAddr,
                    &clientAddr, &clientPeerAddr);

  // The server socket's local address should be the same as the listen
  // address.
  BOOST_CHECK_EQUAL(serverAddr, listenAddr);

  // The client's peer address should always be the same as the server
  // socket's address.
  BOOST_CHECK_EQUAL(clientPeerAddr, serverAddr);

  BOOST_CHECK_EQUAL(serverPeerAddr, clientAddr);
  // Oddly, the address returned by accept() does not seem to match the address
  // returned by getpeername() on the server socket or getsockname() on the
  // client socket.
  // BOOST_CHECK_EQUAL(serverPeerAddr, acceptAddr);
  // BOOST_CHECK_EQUAL(acceptAddr, clientAddr);
}

BOOST_AUTO_TEST_CASE(SetFromSocketUnixExplicit) {
  // Pick two temporary path names.
  // We use mkstemp() just to avoid warnings about mktemp,
  // but we need to remove the file to let the socket code bind to it.
  char serverPath[] = "/tmp/TSocketAddressTest.server.XXXXXX";
  int serverPathFd = mkstemp(serverPath);
  BOOST_REQUIRE_GE(serverPathFd, 0);
  char clientPath[] = "/tmp/TSocketAddressTest.client.XXXXXX";
  int clientPathFd = mkstemp(clientPath);
  BOOST_REQUIRE_GE(clientPathFd, 0);

  int rc = unlink(serverPath);
  BOOST_REQUIRE_EQUAL(rc, 0);
  rc = unlink(clientPath);
  BOOST_REQUIRE_EQUAL(rc, 0);

  TSocketAddress serverBindAddr;
  TSocketAddress clientBindAddr;
  TSocketAddress listenAddr;
  TSocketAddress acceptAddr;
  TSocketAddress serverAddr;
  TSocketAddress serverPeerAddr;
  TSocketAddress clientAddr;
  TSocketAddress clientPeerAddr;
  try {
    serverBindAddr.setFromPath(serverPath);
    clientBindAddr.setFromPath(clientPath);

    testSetFromSocket(&serverBindAddr, &clientBindAddr,
                      &listenAddr, &acceptAddr,
                      &serverAddr, &serverPeerAddr,
                      &clientAddr, &clientPeerAddr);
  } catch (...) {
    // Remove the socket files after we are done
    unlink(serverPath);
    unlink(clientPath);
    throw;
  }
  unlink(serverPath);
  unlink(clientPath);

  // The server socket's local address should be the same as the listen
  // address.
  BOOST_CHECK_EQUAL(serverAddr, listenAddr);

  // The client's peer address should always be the same as the server
  // socket's address.
  BOOST_CHECK_EQUAL(clientPeerAddr, serverAddr);

  BOOST_CHECK_EQUAL(serverPeerAddr, clientAddr);
  BOOST_CHECK_EQUAL(serverPeerAddr, acceptAddr);
  BOOST_CHECK_EQUAL(acceptAddr, clientAddr);
}

BOOST_AUTO_TEST_CASE(SetFromSocketUnixAnonymous) {
  // Test an anonymous client talking to a fixed-path unix socket.
  char serverPath[] = "/tmp/TSocketAddressTest.server.XXXXXX";
  int serverPathFd = mkstemp(serverPath);
  BOOST_REQUIRE_GE(serverPathFd, 0);
  int rc = unlink(serverPath);
  BOOST_REQUIRE_EQUAL(rc, 0);

  TSocketAddress serverBindAddr;
  TSocketAddress listenAddr;
  TSocketAddress acceptAddr;
  TSocketAddress serverAddr;
  TSocketAddress serverPeerAddr;
  TSocketAddress clientAddr;
  TSocketAddress clientPeerAddr;
  try {
    serverBindAddr.setFromPath(serverPath);

    testSetFromSocket(&serverBindAddr, nullptr,
                      &listenAddr, &acceptAddr,
                      &serverAddr, &serverPeerAddr,
                      &clientAddr, &clientPeerAddr);
  } catch (...) {
    // Remove the socket file after we are done
    unlink(serverPath);
    throw;
  }
  unlink(serverPath);

  // The server socket's local address should be the same as the listen
  // address.
  BOOST_CHECK_EQUAL(serverAddr, listenAddr);

  // The client's peer address should always be the same as the server
  // socket's address.
  BOOST_CHECK_EQUAL(clientPeerAddr, serverAddr);

  // Since the client is using an anonymous address, it won't compare equal to
  // any other anonymous addresses.  Make sure the addresses are anonymous.
  BOOST_CHECK_EQUAL(serverPeerAddr.getPath(), "");
  BOOST_CHECK_EQUAL(clientAddr.getPath(), "");
  BOOST_CHECK_EQUAL(acceptAddr.getPath(), "");
}

unit_test::test_suite* init_unit_test_suite(int argc, char* argv[]) {
  unit_test::framework::master_test_suite().p_name.value =
    "TSocketAddressTest";

  if (argc != 1) {
    cerr << "error: unhandled arguments:";
    for (int n = 1; n < argc; ++n) {
      cerr << " " << argv[n];
    }
    cerr << endl;
    exit(1);
  }

  return nullptr;
}
