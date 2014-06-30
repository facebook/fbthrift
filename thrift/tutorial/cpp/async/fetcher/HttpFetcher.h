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
#ifndef THRIFT_TUTORIAL_HTTPFETCHER_H
#define THRIFT_TUTORIAL_HTTPFETCHER_H

#include <string>
#include <functional>

#include <boost/utility.hpp>

#include <thrift/lib/cpp/async/TAsyncSocket.h>

namespace tutorial { namespace async { namespace fetcher {

/**
 * Simple class for asynchronously making an HTTP GET request.
 */
class HttpFetcher :
    private apache::thrift::async::TAsyncTransport::ReadCallback,
    private apache::thrift::async::TAsyncTransport::WriteCallback,
    private boost::noncopyable {
 public:
   /**
    * A callback object to be invoked when the fetch completes successfully.
    *
    * @param _return The contents of the HTTP page.
    */
   typedef std::function<void(std::string const& _return)> ReturnCob;

   /**
    * A callback object to be invoked when the fetch operation fails.
    *
    * @param e The exception raised.
    */
   typedef std::function<void(std::exception const& e)>
     ErrorCob;

   /**
    * Create a new HttpFetcher object.
    *
    * @param event_base The TEventBase to use for performing asynchronous
    *        operations.
    * @param cob The callback object to be invoked on successful completion.
    * @param error_cob The callback object to be invoked if an error occurs.
    * @param ip The HTTP server's IP address.  (This must be an IP address
    *        at the moment, since we do not have support for asynchronously
    *        resolving host names.)
    * @param path The HTTP path to fetch from the server.
    */
   HttpFetcher(apache::thrift::async::TEventBase* event_base,
               ReturnCob cob, ErrorCob error_cob,
               std::string const& ip, std::string const& path) :
       eventBase_(event_base),
       socket_(),
       cob_(cob),
       errorCob_(error_cob),
       ip_(ip),
       path_(path),
       response_(),
       httpRequestLength_(-1) {}

   /**
    * Start the asynchronous fetch operation.
    *
    * This will fetch the HTTP page, and invoke the appropriate callback when
    * the operation is complete.  After invoking the callback, the HttpFetcher
    * object will delete itself.
    *
    * Note that the operation may complete immediately, and invoke the callback
    * before fetch() returns.  In this case the HttpFetcher object will be
    * destroyed by the time fetch() returns.  Callers should be careful to
    * handlt this case correctly, and to not access the HttpFetcher object
    * after fetch() returns.
    */
   void fetch();

 private:
   // TAsyncTransport callback methods
   void writeSuccess() noexcept;
   void writeError(size_t bytesWritten,
                   const apache::thrift::transport::TTransportException& ex)
     noexcept;
   void getReadBuffer(void** bufReturn, size_t* lenReturn);
   void readDataAvailable(size_t len) noexcept;
   void readEOF() noexcept;
   void readError(const apache::thrift::transport::TTransportException& ex)
     noexcept;

   void fail(std::string const& msg);

   apache::thrift::async::TEventBase* eventBase_;
   std::shared_ptr<apache::thrift::async::TAsyncSocket> socket_;
   ReturnCob cob_;
   ErrorCob errorCob_;
   std::string ip_;
   std::string path_;

   std::string response_;
   int httpRequestLength_;
   // Our read and write buffer.
   // We use it for both, since we don't read and write at the same time.
   // We store the HTTP request here while writing.
   // Once we are done writing, we read the response into this buffer,
   // one chunk at a time.
   char buffer_[4096];
};

}}} // tutorial::async::fetcher

#endif // THRIFT_TUTORIAL_HTTPFETCHER_H
