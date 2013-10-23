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
#define BOOST_TEST_MODULE FrozenTest

#include <boost/test/unit_test.hpp>
#include <random>
#include "folly/Conv.h"
#include "folly/Hash.h"
#include "folly/MapUtil.h"
#include "thrift/lib/cpp/protocol/TDebugProtocol.h"
#include "thrift/lib/cpp/test/gen-cpp/FrozenTypes_types.h"

using namespace apache::thrift;
using namespace FrozenTypes;
using std::string;
using std::vector;
using folly::fbstring;
using folly::StringPiece;

auto hasher = std::hash<int64_t>();

double randomDouble(double max) {
  static std::mt19937_64 engine;
  return std::uniform_real_distribution<double>(0, max)(engine);
}

Team testValue() {
  Team team;
  for (int i = 1; i <= 10; ++i) {
    auto id = hasher(i);
    Person p;
    p.id = id;
    p.nums.insert(i);
    p.nums.insert(-i);
    p.dob = randomDouble(1e9);
    folly::toAppend("Person ", i, &p.name);
    team.peopleById[p.id] = p;
    team.peopleByName[p.name] = std::move(p);
  }
  team.projects.insert("alpha");
  team.projects.insert("beta");

  return team;
}

BOOST_AUTO_TEST_SUITE( FrozenTest )

BOOST_AUTO_TEST_CASE( TestFrozen ) {
  Team team = testValue();
  BOOST_CHECK_EQUAL(team.peopleById.at(hasher(3)).name, "Person 3");
  BOOST_CHECK_EQUAL(team.peopleById.at(hasher(4)).name, "Person 4");
  BOOST_CHECK_EQUAL(team.peopleById.at(hasher(5)).name, "Person 5");
  BOOST_CHECK_EQUAL(team.peopleByName.at("Person 3").id, 3);
  BOOST_CHECK_EQUAL(team.peopleByName.begin()->second.nums.count(-1), 1);
  BOOST_CHECK_EQUAL(team.projects.count("alpha"), 1);
  BOOST_CHECK_EQUAL(team.projects.count("beta"), 1);

  size_t size = frozenSize(team);
  for (int misalign = 0; misalign < 16; ++misalign) {
    std::vector<byte> bytes(frozenSize(team) + misalign);
    byte* const freezeLocation = &bytes[misalign];
    byte* buffer = freezeLocation;
    auto* freezeResult = freeze(team, buffer);
    const byte* const frozenLocation =
        static_cast<const byte*>(static_cast<const void*>(freezeResult));
    // verify that freeze didn't yeild a different address.
    BOOST_CHECK_EQUAL(freezeLocation - &bytes[0],
                      frozenLocation - &bytes[0]);

    std::vector<byte> copy(bytes);
    byte* copyBuffer = &bytes[misalign];
    auto& frozen = *(Frozen<Team>*)copyBuffer;

    auto thawedTeam = thaw(frozen);
    BOOST_CHECK_EQUAL(frozen.peopleById.at(hasher(3)).name.range(), "Person 3");
    BOOST_CHECK_EQUAL(frozen.peopleById.at(hasher(4)).name, "Person 4");
    BOOST_CHECK_EQUAL(frozen.peopleById.at(hasher(5)).name, "Person 5");
    BOOST_CHECK_EQUAL(frozen.peopleById.at(hasher(3)).dob,
                      team.peopleById.at(hasher(3)).dob);
    BOOST_CHECK_EQUAL(frozen.peopleByName.at("Person 3").id, 3);
    BOOST_CHECK_EQUAL(frozen.peopleByName.at(string("Person 4")).id, 4);
    BOOST_CHECK_EQUAL(frozen.peopleByName.at(fbstring("Person 5")).id, 5);
    BOOST_CHECK_EQUAL(frozen.peopleByName.at(StringPiece("Person 6")).id, 6);
    BOOST_CHECK_EQUAL(frozen.peopleByName.begin()->second.nums.count(-1), 1);
    BOOST_CHECK_EQUAL(frozen.projects.count("alpha"), 1);
    BOOST_CHECK_EQUAL(frozen.projects.count("beta"), 1);

    BOOST_CHECK_THROW(frozen.peopleById.at(hasher(50)).name, std::out_of_range);
  }
}

BOOST_AUTO_TEST_CASE( FieldOrdering ) {
  Pod p;
  p.a = 0x012345;
  p.b = 0x0678;
  p.c = 0x09;
  BOOST_CHECK_LT(static_cast<void*>(&p.a),
                 static_cast<void*>(&p.b));
  BOOST_CHECK_LT(static_cast<void*>(&p.b),
                 static_cast<void*>(&p.c));
  auto pf = freeze(p);
  auto& f = *pf;
  BOOST_CHECK_EQUAL(sizeof(f.__isset), 1);
  BOOST_CHECK_EQUAL(sizeof(f),
                    sizeof(int32_t) +
                    sizeof(int16_t) +
                    sizeof(uint8_t) +
                    1);
  BOOST_CHECK_LT(static_cast<const void*>(&f.a),
                 static_cast<const void*>(&f.b));
  BOOST_CHECK_LT(static_cast<const void*>(&f.b),
                 static_cast<const void*>(&f.c));
}

BOOST_AUTO_TEST_CASE( IntMap ) {
  std::map<string, int> tmap {
    { "1", 2 },
    { "3", 4 },
    { "7", 8 },
    { "5", 6 },
  };
  auto pfmap = freeze(tmap);
  auto& fmap = *pfmap;
  BOOST_CHECK_EQUAL(fmap.at("3"), 4);
  auto b = fmap.begin();
  auto e = fmap.end();
  using std::make_pair;
  BOOST_CHECK(fmap.find("0") == e);
  BOOST_CHECK(fmap.find("3") == b + 1);
  BOOST_CHECK(fmap.find("4") == e);
  BOOST_CHECK(fmap.find("9") == e);
  BOOST_CHECK(fmap.lower_bound("0") == b);
  BOOST_CHECK(fmap.lower_bound("1") == b);
  BOOST_CHECK(fmap.lower_bound("3") == b + 1);
  BOOST_CHECK(fmap.lower_bound("4") == b + 2);
  BOOST_CHECK(fmap.lower_bound("9") == e);
  BOOST_CHECK(fmap.upper_bound("0") == b);
  BOOST_CHECK(fmap.upper_bound("1") == b + 1);
  BOOST_CHECK(fmap.upper_bound("3") == b + 2);
  BOOST_CHECK(fmap.upper_bound("4") == b + 2);
  BOOST_CHECK(fmap.upper_bound("9") == e);
  BOOST_CHECK(fmap.equal_range("0") == make_pair(b, b));
  BOOST_CHECK(fmap.equal_range("1") == make_pair(b, b + 1));
  BOOST_CHECK(fmap.equal_range("3") == make_pair(b + 1, b + 2));
  BOOST_CHECK(fmap.equal_range("4") == make_pair(b + 2, b + 2));
  BOOST_CHECK(fmap.equal_range("9") == make_pair(e, e));
  BOOST_CHECK(fmap.count("0") == 0);
  BOOST_CHECK(fmap.count("1") == 1);
  BOOST_CHECK(fmap.count("3") == 1);
  BOOST_CHECK(fmap.count("4") == 0);
  BOOST_CHECK(fmap.count("9") == 0);

  BOOST_CHECK_EQUAL(fmap.at("1"), 2);
  BOOST_CHECK_EQUAL(fmap.at("7"), 8);
  BOOST_CHECK_THROW(fmap.at("9"), std::out_of_range);

  BOOST_CHECK(folly::get_ptr(fmap, *freezeStr("1")));
  BOOST_CHECK_EQUAL(2, *folly::get_ptr(fmap, *freezeStr("1")));

  BOOST_CHECK(!folly::get_ptr(fmap, *freezeStr("2")));

  BOOST_CHECK(folly::get_ptr(fmap, *freezeStr("3")));
  BOOST_CHECK_EQUAL(4, *folly::get_ptr(fmap, *freezeStr("3")));
}

BOOST_AUTO_TEST_CASE( Set ) {
  std::set<string> tset { "1", "3", "7", "5" };
  auto pfset = freeze(tset);
  auto& fset = *pfset;
  auto b = fset.begin();
  auto e = fset.end();
  using std::make_pair;
  BOOST_CHECK(fset.find("3") == b + 1);
  BOOST_CHECK(fset.find(folly::fbstring("3")) == b + 1);
  BOOST_CHECK(fset.find(std::string("3")) == b + 1);
  BOOST_CHECK(fset.find(folly::StringPiece("3")) == b + 1);
  BOOST_CHECK(fset.find("0") == e);
  BOOST_CHECK(fset.find("3") == b + 1);
  BOOST_CHECK(fset.find("4") == e);
  BOOST_CHECK(fset.find("9") == e);
  BOOST_CHECK(fset.lower_bound("0") == b);
  BOOST_CHECK(fset.lower_bound("1") == b);
  BOOST_CHECK(fset.lower_bound("3") == b + 1);
  BOOST_CHECK(fset.lower_bound("4") == b + 2);
  BOOST_CHECK(fset.lower_bound("9") == e);
  BOOST_CHECK(fset.upper_bound("0") == b);
  BOOST_CHECK(fset.upper_bound("1") == b + 1);
  BOOST_CHECK(fset.upper_bound("3") == b + 2);
  BOOST_CHECK(fset.upper_bound("4") == b + 2);
  BOOST_CHECK(fset.upper_bound("9") == e);
  BOOST_CHECK(fset.equal_range("0") == make_pair(b, b));
  BOOST_CHECK(fset.equal_range("1") == make_pair(b, b + 1));
  BOOST_CHECK(fset.equal_range("3") == make_pair(b + 1, b + 2));
  BOOST_CHECK(fset.equal_range("4") == make_pair(b + 2, b + 2));
  BOOST_CHECK(fset.equal_range("9") == make_pair(e, e));
  BOOST_CHECK(fset.count("0") == 0);
  BOOST_CHECK(fset.count("1") == 1);
  BOOST_CHECK(fset.count("3") == 1);
  BOOST_CHECK(fset.count("4") == 0);
  BOOST_CHECK(fset.count("9") == 0);
}

BOOST_AUTO_TEST_CASE( Vector ) {
  std::vector<int> tvect { 1, 3, 7, 5 };
  auto pfvect = freeze(tvect);
  auto& fvect = *pfvect;
  auto b = fvect.begin();
  auto e = fvect.end();
  using std::make_pair;
  BOOST_CHECK_EQUAL(fvect.front(), 1);
  BOOST_CHECK_EQUAL(fvect.back(), 5);
  BOOST_CHECK_EQUAL(fvect.size(), 4);
  BOOST_CHECK_EQUAL(fvect[1], 3);
  BOOST_CHECK_EQUAL(b[0], 1);
  BOOST_CHECK_EQUAL(b[1], 3);
  BOOST_CHECK_EQUAL(e[-1], 5);
  BOOST_CHECK_EQUAL(e[-2], 7);
}

BOOST_AUTO_TEST_SUITE_END()
