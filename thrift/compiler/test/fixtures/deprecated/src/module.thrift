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

typedef i64 ColorID

struct House {
  1: ColorID id,
  2: string houseName,
  3: optional set<ColorID> houseColors (deprecated="Not used anymore")
} (deprecated)

struct Field {
  1: ColorID id,
  2: i32 fieldType = 5 (deprecated)
} (deprecated="No longer supported")
