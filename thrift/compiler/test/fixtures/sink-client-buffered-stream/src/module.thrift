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

struct InitialResponse {
  1: string content;
}

struct FinalResponse {
  1: string content;
}

struct SinkPayload {
  1: string content;
}

struct CompatibleWithKeywordSink {
  // sink keyword only has effect in service scope
  1: string sink;
}

exception SinkException {
  1: string reason;
}

service SinkService {
  sink<SinkPayload, FinalResponse> method();
  InitialResponse, sink<SinkPayload, FinalResponse> methodAndReponse();
  sink<SinkPayload, FinalResponse> methodThrow() throws (1: SinkException ex);
}
