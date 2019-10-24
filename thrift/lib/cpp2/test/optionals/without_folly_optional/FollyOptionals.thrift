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

enum HasOptionalsTestEnum {
  FOO = 1,
  BAR = 2,
}

struct HasOptionalsExtra {
  1: i64 extraInt64Req;
  2: optional i64 extraInt64Opt;
  3: string extraStringReq;
  4: optional string extraStringOpt;
  5: set<i64> extraSetReq;
  6: optional set<i64> extraSetOpt;
  7: list<i64> extraListReq;
  8: optional list<i64> extraListOpt;
  9: map<i64, i64> extraMapReq;
  10: optional map<i64, i64> extraMapOpt;
  11: HasOptionalsTestEnum extraEnumReq;
  12: optional HasOptionalsTestEnum extraEnumOpt;
}

struct HasOptionals {
  1: i64 int64Req;
  2: optional i64 int64Opt;
  3: string stringReq;
  4: optional string stringOpt;
  5: set<i64> setReq;
  6: optional set<i64> setOpt;
  7: list<i64> listReq;
  8: optional list<i64> listOpt;
  9: map<i64, i64> mapReq;
  10: optional map<i64, i64> mapOpt;
  11: HasOptionalsTestEnum enumReq;
  12: optional HasOptionalsTestEnum enumOpt;
  13: HasOptionalsExtra structReq;
  14: optional HasOptionalsExtra structOpt;
}
