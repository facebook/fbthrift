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
#include "thrift/lib/cpp/test/loadgen/LatencyScoreBoard.h"

#include "thrift/lib/cpp/concurrency/Util.h"

#include <math.h>

namespace apache { namespace thrift { namespace loadgen {

/*
 * LatencyScoreBoard::OpData methods
 */

LatencyScoreBoard::OpData::OpData() {
  zero();
}

void LatencyScoreBoard::OpData::addDataPoint(uint64_t latency) {
  uint64_t oldCount = count_;
  uint64_t oldUsecSum = usecSum_;
  ++count_;
  usecSum_ += latency;
  sumOfSquares_ += latency*latency;
}

void LatencyScoreBoard::OpData::zero() {
  count_ = 0;
  usecSum_ = 0;
  sumOfSquares_ = 0;
}

void LatencyScoreBoard::OpData::accumulate(const OpData* other) {
  count_ += other->count_;
  usecSum_ += other->usecSum_;
  sumOfSquares_ += other->sumOfSquares_;
}

uint64_t LatencyScoreBoard::OpData::getCount() const {
  return count_;
}

uint64_t LatencyScoreBoard::OpData::getCountSince(const OpData* other) const {
  return count_ - other->count_;
}

double LatencyScoreBoard::OpData::getLatencyAvg() const {
  if (count_ == 0) {
    return 0;
  }
  return static_cast<double>(usecSum_) / count_;
}

double LatencyScoreBoard::OpData::getLatencyAvgSince(
    const OpData* other) const {
  if (other->count_ >= count_) {
    return 0;
  }
  return (static_cast<double>(usecSum_ - other->usecSum_) /
          (count_ - other->count_));
}

double LatencyScoreBoard::OpData::getLatencyStdDev() const {
  if (count_ == 0) {
    return 0;
  }
  return sqrt((sumOfSquares_ - usecSum_ * (usecSum_ / count_)) / count_);
}

double LatencyScoreBoard::OpData::getLatencyStdDevSince(
    const OpData* other) const {
  if (other->count_ >= count_) {
    return 0;
  }

  uint64_t deltaSumOfSquares = sumOfSquares_ - other->sumOfSquares_;
  uint64_t deltaCount = count_ - other->count_;
  uint64_t deltaSum = usecSum_ - other->usecSum_;
  return sqrt((deltaSumOfSquares - deltaSum *
              (deltaSum / deltaCount)) / deltaCount);
}

/*
 * LatencyScoreBoard methods
 */

void LatencyScoreBoard::opStarted(uint32_t opType) {
  startTime_ = concurrency::Util::currentTimeUsec();
}

void LatencyScoreBoard::opSucceeded(uint32_t opType) {
  OpData* data = opData_.getOpData(opType);

  uint64_t latency = (concurrency::Util::currentTimeUsec() - startTime_);
  data->addDataPoint(latency);
}

void LatencyScoreBoard::opFailed(uint32_t opType) {
}

const LatencyScoreBoard::OpData* LatencyScoreBoard::getOpData(uint32_t opType) {
  return opData_.getOpData(opType);
}

void LatencyScoreBoard::computeOpAggregate(OpData* result) const {
  opData_.accumulateOverOps(result);
}

void LatencyScoreBoard::zero() {
  opData_.zero();
}

void LatencyScoreBoard::accumulate(const LatencyScoreBoard* other) {
  opData_.accumulate(&other->opData_);
}

}}} // apache::thrift::loadgen
