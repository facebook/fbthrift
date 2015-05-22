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

#ifndef _THRIFT_TSYNCTOASYNCPROCESSOR_H_
#define _THRIFT_TSYNCTOASYNCPROCESSOR_H_ 1

#include <functional>
#include <memory>
#include <thrift/lib/cpp/TProcessor.h>
#include <thrift/lib/cpp/async/TAsyncProcessor.h>

namespace apache { namespace thrift { namespace async {

/**
 * Adapter to allow a TProcessor to be used as a TAsyncProcessor.
 *
 * Note that this should only be used for handlers that return quickly without
 * blocking, since async servers can be stalled by a single blocking operation.
 */
class TSyncToAsyncProcessor : public TAsyncProcessor {
 public:
  TSyncToAsyncProcessor(std::shared_ptr<TProcessor> processor)
    : processor_(processor)
  {}

  void process(std::function<void(bool success)> _return,
               std::shared_ptr<protocol::TProtocol> in,
               std::shared_ptr<protocol::TProtocol> out,
               TConnectionContext* context) override {
    return _return(processor_->process(in, out, context));
  }

 private:
  std::shared_ptr<TProcessor> processor_;
};

}}} // apache::thrift::async

#endif // #ifndef _THRIFT_TSYNCTOASYNCPROCESSOR_H_
