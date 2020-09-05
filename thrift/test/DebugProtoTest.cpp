/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cmath>
#include <iostream>

#include <thrift/lib/cpp2/protocol/DebugProtocol.h>
#include <thrift/test/gen-cpp2/DebugProtoTest_constants.h>
#include <thrift/test/gen-cpp2/DebugProtoTest_types_custom_protocol.h>

using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::test;

int main() {
  OneOfEach ooe;
  *ooe.im_true_ref() = true;
  *ooe.im_false_ref() = false;
  *ooe.a_bite_ref() = 0xd6;
  *ooe.integer16_ref() = 27000;
  *ooe.integer32_ref() = 1 << 24;
  *ooe.integer64_ref() = (uint64_t)6000 * 1000 * 1000;
  *ooe.double_precision_ref() = M_PI;
  *ooe.some_characters_ref() = "Debug THIS!";
  *ooe.zomg_unicode_ref() = "\xd7\n\a\t";
  ooe.string_string_map_ref()["one"] = "two";
  ooe.string_string_hash_map_ref()["three"] = "four";
  ooe.string_set_ref()->insert("five");
  ooe.string_hash_set_ref()->insert("six");
  *ooe.float_precision_ref() = (float)12.345;
  ooe.rank_map_ref()[567419810] = (float)0.211184;
  ooe.rank_map_ref()[507959914] = (float)0.080382;

  cout << debugString(ooe) << endl << endl;

  cout << "--- const1" << endl;
  cout << debugString(DebugProtoTest_constants::const1()) << endl << endl;
  cout << "--- const2" << endl;
  cout << debugString(DebugProtoTest_constants::const2()) << endl << endl;

  Nesting n;
  *n.my_ooe_ref() = ooe;
  *n.my_ooe_ref()->integer16_ref() = 16;
  *n.my_ooe_ref()->integer32_ref() = 32;
  *n.my_ooe_ref()->integer64_ref() = 64;
  *n.my_ooe_ref()->double_precision_ref() = (std::sqrt(5.0) + 1) / 2;
  *n.my_ooe_ref()->some_characters_ref() = ":R (me going \"rrrr\")";
  *n.my_ooe_ref()->zomg_unicode_ref() =
      "\xd3\x80\xe2\x85\xae\xce\x9d\x20"
      "\xd0\x9d\xce\xbf\xe2\x85\xbf\xd0\xbe\xc9\xa1\xd0\xb3\xd0\xb0\xcf\x81\xe2\x84\x8e"
      "\x20\xce\x91\x74\x74\xce\xb1\xe2\x85\xbd\xce\xba\xc7\x83\xe2\x80\xbc";
  *n.my_bonk_ref()->type_ref() = 31337;
  *n.my_bonk_ref()->message_ref() = "I am a bonk... xor!";

  cout << debugString(n) << endl << endl;

  HolyMoley hm;

  hm.big_ref()->push_back(ooe);
  hm.big_ref()->push_back(*n.my_ooe_ref());
  *hm.big_ref()[0].a_bite_ref() = 0x22;
  *hm.big_ref()[1].a_bite_ref() = 0x33;

  std::vector<std::string> stage1;
  stage1.push_back("and a one");
  stage1.push_back("and a two");
  hm.contain_ref()->insert(stage1);
  stage1.clear();
  stage1.push_back("then a one, two");
  stage1.push_back("three!");
  stage1.push_back("FOUR!!");
  hm.contain_ref()->insert(stage1);
  stage1.clear();
  hm.contain_ref()->insert(stage1);

  std::vector<Bonk> stage2;
  hm.bonks_ref()["nothing"] = stage2;
  stage2.resize(stage2.size() + 1);
  *stage2.back().type_ref() = 1;
  *stage2.back().message_ref() = "Wait.";
  stage2.resize(stage2.size() + 1);
  *stage2.back().type_ref() = 2;
  *stage2.back().message_ref() = "What?";
  hm.bonks_ref()["something"] = stage2;
  stage2.clear();
  stage2.resize(stage2.size() + 1);
  *stage2.back().type_ref() = 3;
  *stage2.back().message_ref() = "quoth";
  stage2.resize(stage2.size() + 1);
  *stage2.back().type_ref() = 4;
  *stage2.back().message_ref() = "the raven";
  stage2.resize(stage2.size() + 1);
  *stage2.back().type_ref() = 5;
  *stage2.back().message_ref() = "nevermore";
  hm.bonks_ref()["poe"] = stage2;

  cout << debugString(hm) << endl << endl;

  TestUnion u;
  u.set_struct_field(std::move(ooe));
  cout << debugString(u) << endl << endl;

  return 0;
}
