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
#include "thrift/tutorial/cpp/async/sort/SortDistributorHandler.h"

#include <queue>

#include "thrift/lib/cpp/async/TAsyncSocket.h"
#include "thrift/lib/cpp/async/TFramedAsyncChannel.h"
#include "thrift/lib/cpp/protocol/TBinaryProtocol.h"
#include "thrift/lib/cpp/transport/TBufferTransports.h"

using std::string;
using namespace boost;
using namespace apache::thrift;
using namespace apache::thrift::async;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

namespace tutorial { namespace sort {

/**
 * This class waits on all of the responses to be received.
 *
 * For each backend server that we send a request to, sortDone() will be called
 * with the response from that server.  When all of the individually sorted
 * chunks have been received, Aggregator merges them into a single vector and
 * invokes the callback object.
 */
class SortDistributorHandler::Aggregator {
 public:
  /**
   * Create a new Aggregator.
   *
   * @param num_servers The number of servers to which requests will be sent.
   *     sortDone() should be called exactly num_servers times.
   * @param cob The callback object to invoke with the merged list.
   */
  Aggregator(TEventBase* ev_base, SortReturnCob cob, SortErrorCob errcob) :
    allSent_(false),
    numPending_(0),
    eventBase_(ev_base),
    cob_(cob),
    errorCob_(errcob) { }

  /**
   * Send a request to sort a chunk.
   *
   * This method should only be invoked before the call to allChunksSent().
   */
  void sortChunk(const IntVector& chunk, const std::string& ip, uint16_t port);

  /**
   * Indicate that all sortChunk() requests have been started.
   *
   * Once all of the individual sort operations have completed, the Aggregator
   * will invoke the appropriate callback and delete itself.
   *
   * Note that if all sort operations have already completed when
   * allChunksSent() is called, the callback will be invoked before
   * allChunksSent() returns.  In this case, the Aggregator will also be
   * destroyed before allChunksSent() returns.  Callers must be careful to
   * handle this case correctly, and to not access the Aggregator again after
   * allChunksSent() returns.
   */
  void allChunksSent();

 private:
  // convenience typedef, just for a shorter name
  typedef SorterCobClientT< TBinaryProtocolT<TBufferBase> > AsyncSortClient;

  /**
   * VectorPointer stores an iterator into one of our response vectors,
   * and an iterator pointing to the end of the same response vector.
   *
   * We use this when merging all of the responses back into a single list.
   */
  struct VectorPointer {
    VectorPointer(const IntVector* v) : it_(v->begin()), end_(v->end()) {}

    IntVector::const_iterator it_;
    IntVector::const_iterator end_;
  };
  struct VPCompare {
    bool operator()(const VectorPointer& vp1, const VectorPointer& vp2) {
      return *vp1.it_ > *vp2.it_;
    }
  };


  void sortDone(AsyncSortClient* client);
  void checkDone();
  void invokeCob();

  // Have we sent all chunks out?
  bool allSent_;
  // The number of requests still pending.
  unsigned int numPending_;
  // The TEventBase used to drive our operation
  TEventBase* eventBase_;
  // The callback to invoke when we have the fully sorted list ready.
  SortReturnCob cob_;
  // The callback to invoke if an error occurs
  SortErrorCob errorCob_;
  // The list individually sorted chunks that we have received so far.
  std::vector<IntVector> responses_;
};

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
void SortDistributorHandler::sort(SortReturnCob cob, SortErrorCob errcob,
                                  const IntVector &values) {
  // If there's just one value, go ahead and return it now.
  // (This avoids infinite recursion if we happen to be pointing at ourself as
  // one of the backend servers.)
  if (values.size() <= 1) {
    cob(values);
    return;
  }

  // Perform the sort by breaking the list into pieces,
  // and farming them out the the servers we know about.
  size_t chunk_size = values.size() / sortServers_.size();

  // Round up the chunk size when it is not integral
  if (values.size() % sortServers_.size() != 0) {
    chunk_size += 1;
  }

  // Create a callback to aggregate all of the results
  Aggregator* aggregator = new Aggregator(getEventBase(), cob, errcob);

  IntVector::const_iterator chunk_start = values.begin();
  IntVector::const_iterator chunk_end = chunk_start + chunk_size;
  for (ServerVector::const_iterator it = sortServers_.begin();
       it != sortServers_.end();
       ++it) {
    // When the list does not evenly divide into the number of servers,
    // give the extras to the last server.  (In the real world, we would
    // probably want to distribute this more evenly over the servers, instead
    // of always giving the last server extra work.)
    if (it + 1 == sortServers_.end()) {
      chunk_end = values.end();
    }

    IntVector chunk(chunk_start, chunk_end);
    aggregator->sortChunk(chunk, it->first, it->second);

    chunk_start = chunk_end;
    chunk_end = chunk_start + chunk_size;

    // Since we rounded chunk_size up, make sure chunk_end doesn't go past the
    // end of the list.
    if (chunk_end > values.end()) {
      chunk_end = values.end();
    }

    // When the number of elements to sort is less than the number of servers,
    // we might not have any data for the last few servers to parse at all.
    if (chunk_start >= values.end()) {
      break;
    }
  }

  // Tell the aggregator that all chunks have been sent,
  // so it can merge the results as soon as it has all responses.
  aggregator->allChunksSent();
}

/**
 * Send a request to another server to sort a chunk of the integer list
 */
void SortDistributorHandler::Aggregator::sortChunk(const IntVector& values,
                                                   const string& ip,
                                                   uint16_t port) {
  // Create a TAsyncSocket to the desired server.
  // This will asynchronously begin the connect.  Attempts to read/write to the
  // socket will be queued until the connect completes, so we can start using
  // it right away, even though it isn't connected yet.
  int connect_timeout_ms = 1000;
  std::shared_ptr<TAsyncSocket> socket =
    TAsyncSocket::newSocket(eventBase_, ip, port, connect_timeout_ms);

  // Create the thrift client
  std::shared_ptr<TFramedAsyncChannel> channel =
    TFramedAsyncChannel::newChannel(socket);
  TBinaryProtocolFactoryT<TBufferBase> proto_factory;
  AsyncSortClient* client = new AsyncSortClient(channel, &proto_factory);

  // Increment the number of pending calls
  ++numPending_;

  // Make the call.  This simply starts the call; we should now return, which
  // will give back control to the TEventBase loop.  When the response is later
  // received, the callback object will be invoked.
  client->sort(std::bind(&Aggregator::sortDone, this,
                              std::placeholders::_1),
               values);
}

void SortDistributorHandler::Aggregator::allChunksSent() {
  allSent_ = true;

  // Check to see if we're done, in case we've already got all of the responses
  // (This probably won't ever happen in practice.)
  checkDone();
}

/**
 * This method is invoked when a remote server returns a response
 * containing one of the sorted list chunks.
 */
void SortDistributorHandler::Aggregator::sortDone(AsyncSortClient* client) {
  // If an error occurred on a previous response, we will have already invoked
  // the error callback and set cob_ to NULL.  If cob_ is unset, don't bother
  // parsing the response, just skip to the clean up.
  if (cob_) {
    // Call client->recv_sort() to parse the response and extract the return
    // value.
    //
    // Although it might seem more convenient if thrift were to parse the
    // response for us and pass in the return value, that would also force an
    // extra data copy.  If the vector were passed in as an argument, we would
    // be forced to make a copy of it.  Instead, since we call recv_sort()
    // ourself, we can parse the result directly into our own persistent
    // vector.
    responses_.push_back(IntVector());
    try {
      client->recv_sort(responses_.back());
    } catch (TException& ex) {
      // An error occurred.  Go ahead and invoke the error callback now with the
      // error, so we can inform the client of the failure.
      //
      // The Aggregator itself will stay around until sortDone() is invoked for
      // the remaining sort responses.  We set errorCob_ and cob_ to NULL so
      // that we'll know that we're just performing cleanup in later sortDone()
      // calls.
      //
      // Depending on the application requirements, a smarter implementation
      // could try to perform some sort of error recovery here.  e.g.,
      // re-sending the request to a different backend server, or performing
      // the sort locally.
      SortErrorCob tmp_errcob = errorCob_;
      errorCob_ = NULL;
      cob_ = NULL;

      // Convert the exception to a SortError so the client will be able to
      // catch it.
      SortError sorterr;
      sorterr.msg = std::string("error performing distributed sort: ") +
        ex.what();
      tmp_errcob(sorterr);
    }
  }

  // The AsyncSortClient was dynamically allocated when we created the request.
  // We aren't going to use it anymore, so delete it now.
  delete client;

  // Decrement the number of pending responses that we are waiting on
  --numPending_;

  // Check to see if we have all the responses
  checkDone();
}

/**
 * This method is called after each call to sortDone().
 *
 * If we are still waiting on other servers to respond, it returns without
 * doing anything.  If all responses have been received, it merges the
 * individually sorted lists back into one list, and invokes the callback with
 * the result.
 */
void SortDistributorHandler::Aggregator::checkDone() {
  // If we still have more chunks to send, we aren't done yet.
  if (!allSent_) {
    return;
  }
  // If we are still waiting on more responses, we aren't done.
  if (numPending_ > 0) {
    return;
  }

  // cob_ may be NULL if an error occurred and we've
  // already invoked errorCob_.
  //
  // If cob_ is still non-NULL, all the responses were successful.
  if (cob_) {
    // Merge the results and invoke the normal callback
    invokeCob();
  }

  // We were allocated on the heap in SortDistributorHandler::sort()
  // Destroy ourself now that we have completed the operation.
  delete this;
}

void
SortDistributorHandler::Aggregator::invokeCob() {
  // Merge the sorted chunks back into one list.
  //
  // Build a heap containing one iterator for each of the response vectors.
  // The iterator pointing to the smallest value is kept on top.
  typedef std::priority_queue<VectorPointer, std::vector<VectorPointer>,
                              VPCompare> IteratorHeap;
  IteratorHeap h;
  for (std::vector<IntVector>::const_iterator it = responses_.begin();
       it != responses_.end();
       ++it) {
    if (it->empty()) {
      continue;
    }

    h.push(VectorPointer(&*it));
  }

  IntVector merged;
  while (!h.empty()) {
    // Pop off the vector with the smallest next element
    VectorPointer vp = h.top();
    h.pop();

    // Add this element to the merged vector
    merged.push_back(*vp.it_);

    // Increment the iterator for that vector
    ++vp.it_;
    // If we aren't at the end of this vector,
    // put the VectorPointer back in the heap
    if (vp.it_ != vp.end_) {
      h.push(vp);
    }
  }

  // Call the callback with the final result.
  cob_(merged);
}

}} // tutorial::sort
