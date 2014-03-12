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

#ifndef TNOTIFICATIONQUEUE_H
#define TNOTIFICATIONQUEUE_H

#include "folly/io/async/NotificationQueue.h"
#include "thrift/lib/cpp/async/TEventBase.h"
#include "thrift/lib/cpp/Thrift.h"

namespace apache { namespace thrift { namespace async {

template <typename MessageT>
using TNotificationQueue = folly::NotificationQueue<MessageT>;

}}}

#endif
