/*
 * Copyright 2014 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define BOOST_TEST_MODULE FrozenUtilTest

#include <boost/test/unit_test.hpp>

#include <folly/File.h>
#include <folly/MemoryMapping.h>

#include <thrift/lib/cpp/protocol/TDebugProtocol.h>
#include <thrift/lib/cpp/test/gen-cpp/FrozenTypes_types.h>
#include <thrift/lib/cpp/util/FrozenUtil.h>
#include <thrift/lib/cpp/util/FrozenTestUtil.h>

using namespace apache::thrift;
using namespace apache::thrift::util;
using namespace FrozenTypes;
using folly::File;
using folly::MemoryMapping;

namespace {

double filesize(int fd) {
  struct stat st;
  fstat(fd, &st);
  return st.st_size;
}

}
BOOST_AUTO_TEST_SUITE( FrozenUtilTest )

BOOST_AUTO_TEST_CASE( Set ) {
  std::set<std::string> tset { "1", "3", "7", "5" };
  auto tempFrozen = freezeToTempFile(tset);
  MemoryMapping mapping(tempFrozen.fd());

  auto* pfset = mapFrozen<std::set<std::string>>(mapping);
  auto& fset = *pfset;
  BOOST_CHECK_EQUAL(1, fset.count("3"));
  BOOST_CHECK_EQUAL(0, fset.count("4"));
}

BOOST_AUTO_TEST_CASE( Vector ) {
  std::vector<Person> people(3);
  people[0].id = 300;
  people[1].id = 301;
  people[2].id = 302;

  auto tempFrozen = freezeToTempFile(people);
  MemoryMapping mapping(tempFrozen.fd());

  auto* pfvect= mapFrozen<std::vector<Person>>(mapping);
  auto& fvect = *pfvect;
  BOOST_CHECK_EQUAL(300, fvect[0].id);
  BOOST_CHECK_EQUAL(302, fvect[2].id);
}

BOOST_AUTO_TEST_CASE( Shrink ) {
  std::vector<Person> people(3);

  File f = File::temporary();

  size_t count = 1 << 16;
  for (int i = 0; i < count; ++i) {
    people.emplace_back();
    people.back().id = i + count;
  }

  freezeToFile(people, f.fd());
  BOOST_CHECK_CLOSE(sizeof(Frozen<Person>) * count,
                    filesize(f.fd()),
                    1);

  count /= 16;
  people.resize(count);

  freezeToFile(people, f.fd());
  BOOST_CHECK_CLOSE(sizeof(Frozen<Person>) * count,
                    filesize(f.fd()),
                    1);
}

BOOST_AUTO_TEST_CASE( Sparse ) {
  std::vector<Person> people;

  size_t count = 1 << 20;
  for (int i = 0; i < count; ++i) {
    people.emplace_back();
    people.back().id = i + count;
  }

  File f = File::temporary();

  freezeToSparseFile(people, folly::File(f.fd()));

  BOOST_CHECK_CLOSE(sizeof(Frozen<Person>) * count,
                    filesize(f.fd()),
                    1);

  MemoryMapping mapping(f.fd());
  auto* pfvect= mapFrozen<std::vector<Person>>(mapping);
  auto& fvect = *pfvect;
  BOOST_CHECK_EQUAL(people[100].id, fvect[100].id);
  BOOST_CHECK_EQUAL(people[9876].id, fvect[9876].id);
}

BOOST_AUTO_TEST_CASE( KeepMapped ) {
  Person p;
  p.nums = { 9, 8, 7 };
  p.id = 123;
  p.name = "Tom";

  File f = File::temporary();
  MemoryMapping mapping(folly::File(f.fd()), 0, frozenSize(p),
                        MemoryMapping::writable());

  //also returns mapped addr
  auto* pfp = freezeToFile(p, mapping);
  auto& fp = *pfp;

  BOOST_CHECK_EQUAL(123, fp.id);
  BOOST_CHECK_EQUAL(1, fp.nums.count(8));
  BOOST_CHECK_EQUAL(3, fp.nums.size());
  BOOST_CHECK_EQUAL("Tom", fp.name);
}

BOOST_AUTO_TEST_SUITE_END()
