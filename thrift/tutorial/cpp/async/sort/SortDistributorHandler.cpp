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

#include <thrift/tutorial/cpp/async/sort/SortDistributorHandler.h>

#include <queue>
#include <folly/gen/Base.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>

using namespace std;
using namespace folly;

namespace apache { namespace thrift { namespace tutorial { namespace sort {

/*
 * Distributed sorting.
 *
 * Performs the sorting by breaking the list into chunks, sending requests out
 * to each backend server to sort 1 chunk, then merging the results.
 *
 * Sorting a list of size N normally requires O(N log N) time.
 * Distributing the sorting operation over M servers requires
 * O(N/M log N/M) time on each server (performed in parallel), plus
 * O(N log M) time to merge the sorted lists back together.
 *
 * (In reality, the extra I/O overhead of copying the data and sending it out
 * to the servers probably makes it not worthwhile for most use cases.
 * However, it provides a relatively easy-to-understand example.)
 */
Future<vector<int32_t>> SortDistributorHandler::future_sort(
    const vector<int32_t>& values) {
  // If there's just one value, go ahead and return it now.
  // (This avoids infinite recursion if we happen to be pointing at ourself as
  // one of the backend servers.)
  if (values.size() <= 1) {
    return makeFuture(values);
  }

  // Perform the sort by breaking the list into pieces,
  // and farming them out the the servers we know about.
  size_t chunk_size = values.size() / backends_.size();

  // Round up the chunk size when it is not integral
  if (values.size() % backends_.size() != 0) {
    chunk_size += 1;
  }

  auto tm = getThreadManager();
  auto eb = getEventBase();

  // Create futures for all the requests to the backends, and when they all
  // complete.
  return collectAll(
    gen::range<size_t>(0, backends_.size())
    | gen::map([&](size_t idx) {
        // Chunk it.
        auto a = chunk_size * idx;
        auto b = chunk_size * (idx + 1);
        vector<int32_t> chunk(
            values.begin() + a,
            values.begin() + std::min(values.size(), b));
        // Issue a request from the IO thread for each chunk.
        auto chunkm = makeMoveWrapper(move(chunk));
        return via(eb, [=]() mutable {
            auto chunk = chunkm.move();
            auto client = make_unique<SorterAsyncClient>(
                HeaderClientChannel::newChannel(
                  async::TAsyncSocket::newSocket(
                    eb, backends_.at(idx))));
            return client->future_sort(chunk);
        });
      })
    | gen::as<vector>())
    // Back in a CPU thread, when al the results are in.
    .via(tm)
    .then([=](vector<Try<vector<int32_t>>> xresults) {
        // Throw if any of the backends threw.
        auto results = gen::from(xresults)
          | gen::map([](Try<vector<int32_t>> xresult) {
              return move(xresult.value());
            })
          | gen::as<vector>();

        // Build a heap containing one Range for each of the response vectors.
        // The Range starting with the smallest value is kept on top.
        using it = vector<int32_t>::const_iterator;
        struct it_range_cmp {
          bool operator()(Range<it> a, Range<it> b) {
            return a.front() > b.front();
          }
        };
        priority_queue<Range<it>, vector<Range<it>>, it_range_cmp> heap;
        for (auto& result : results) {
          if (result.empty()) {
            continue;
          }
          heap.push(Range<it>(result.begin(), result.end()));
        }

        // Cycle through the heap, merging the sorted chunks back into one list.
        // On each iteration:
        // * Pull out the Range with the least first element (each Range is pre-
        //   sorted).
        // * Pull out that first element and add it to the merged result.
        // * Put the Range back without that first element, if it is non-empty.
        vector<int32_t> merged;
        while (!heap.empty()) {
          auto v = heap.top();
          heap.pop();
          merged.push_back(v.front());
          v.advance(1);
          if (!v.empty()) {
            heap.push(v);
          }
        }

        return merged;
    });
}

}}}}
