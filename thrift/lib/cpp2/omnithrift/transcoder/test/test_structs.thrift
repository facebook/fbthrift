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

namespace cpp2 transcoder.test_structs
namespace py3 omnithrift.py

enum Continent {
  NorthAmerica = 1,
  SouthAmerica = 2,
  Europe = 3,
  Asia = 4,
  Africa = 5,
  Oceania = 6,
  Antarctica = 7,
}

struct Country {
  1: string name;
  2: Continent continent;
  3: string capital;
  4: double population;
}

struct City {
  1: string name;
  2: Country country;
}

struct Request {
  1: list<City> cityList;
  2: set<string> stringSet;
  3: map<string, City> cityMap;
}

exception Warning {
  1: string message;
  2: i32 code;
}

service TestService {
  Country getCountry(1: Request req) throws (1: Warning ex);
}
