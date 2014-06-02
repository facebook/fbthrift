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

// Public D interface

class CppTEventBase {
  this(TEventBase* eb) {
    eb_ = eb;
  }

  void loop() {
    teventbase_loop(eb_);
  }

  bool isRunning() {
    return teventbase_isRunning(eb_);
  }

  private:
    TEventBase* eb_;
}

// C++ logic follows

// C++ Pointers
struct TEventBase;

// C++ interface
extern (C) {
  void teventbase_loop(TEventBase*);
  bool teventbase_isRunning(TEventBase*);
}
