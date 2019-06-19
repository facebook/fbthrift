/*
 * Copyright 2018-present Facebook, Inc.
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

#pragma once

namespace apache {
namespace thrift {

class StreamClientCallback;

namespace rocket {

class RequestFnfFrame;
class RequestResponseFrame;
class RequestStreamFrame;
class SetupFrame;

class RocketServerFrameContext;

class RocketServerHandler {
 public:
  virtual ~RocketServerHandler() = default;

  virtual void handleSetupFrame(SetupFrame&&, RocketServerFrameContext&&) {}

  virtual void handleRequestResponseFrame(
      RequestResponseFrame&& frame,
      RocketServerFrameContext&& context) = 0;

  virtual void handleRequestFnfFrame(
      RequestFnfFrame&& frame,
      RocketServerFrameContext&& context) = 0;

  virtual void handleRequestStreamFrame(
      RequestStreamFrame&&,
      StreamClientCallback*) = 0;
};

} // namespace rocket
} // namespace thrift
} // namespace apache
