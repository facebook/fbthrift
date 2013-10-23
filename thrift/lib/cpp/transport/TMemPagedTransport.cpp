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

#include "thrift/lib/cpp/transport/TMemPagedTransport.tcc"

namespace apache { namespace thrift { namespace transport {

using std::shared_ptr;

/**
 * Constructor
 */
TMemPagedTransport::TMemPagedTransport(
                        shared_ptr<FixedSizeMemoryPageFactory>& factory) :
  factory_(factory),
  pageSize_(factory->getPageSize()),
  headPage_(nullptr),
  currentReadPage_(nullptr),
  currentReadOffset_(0),
  currentWritePage_(nullptr),
  currentWriteOffset_(0) {
}

/**
 * Destructor
 */
TMemPagedTransport::~TMemPagedTransport() {
  resetForWrite(true);
}


/**
 * Whether this transport is open.
 */
bool TMemPagedTransport::isOpen() {
  return true;
}

/**
 * Opens the transport.
 */
void TMemPagedTransport::open() {
}

/**
 * Closes the transport.
 */
void TMemPagedTransport::close() {
}

/**
 * Called when read is completed.
 * This can be over-ridden to perform a transport-specific action
 * e.g. logging the request to a file
 *
 * @return number of bytes read if available, 0 otherwise.
 */
uint32_t TMemPagedTransport::readEnd() {
  uint32_t readOffset = getReadBytes();
  uint32_t writeOffset = getWrittenBytes();
  // reset buffer for write when we done with read
  if (readOffset == writeOffset) {
    resetForWrite();
  }
  return readOffset;
}

/**
 * Called when write is completed.
 * This can be over-ridden to perform a transport-specific action
 * at the end of a request.
 *
 * @return number of bytes written if available, 0 otherwise
 */
uint32_t TMemPagedTransport::writeEnd() {
  uint32_t writeOffset = getWrittenBytes();
  return writeOffset;
}


}}} // apache::thrift::transport
