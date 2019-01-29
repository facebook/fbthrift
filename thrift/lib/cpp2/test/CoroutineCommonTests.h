/*
 * Copyright 2019 Facebook, Inc.
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

// We want to perform the same set of client tests against the implementations
// of CoroutineSvIf in CoroutineTest.cpp and CoroutineNoSupportTest.cpp.
// Ordinarily, we'd do these with GTest's parameterized test functionality.
// However, we want to make sure that you can still implement a coroutine-ified
// interface even on a compiler that doesn't support coroutines. To do this, we
// need to pass different compiler flags, and so need different build targets.
// To share the client test code, we pull it into its own file, and let each
// test file include it.

class CoroutineTest : public testing::Test {
 public:
  CoroutineTest()
      : ssit_(std::make_shared<CoroutineServiceHandler>()),
        client_(ssit_.newClient<CoroutineAsyncClient>(
            *EventBaseManager::get()->getEventBase())) {}

 protected:
  template <typename Func>
  void expectSumResults(Func computeSum) {
    for (int i = 0; i < 10; ++i) {
      for (int j = 0; j < 10; ++j) {
        EXPECT_EQ(i + j, computeSum(i, j));
      }
    }
  }
  ScopedServerInterfaceThread ssit_;
  std::unique_ptr<CoroutineAsyncClient> client_;
};

TEST_F(CoroutineTest, SumNoCoro) {
  expectSumResults([&](int x, int y) {
    SumRequest request;
    request.x = x;
    request.y = y;
    SumResponse response;
    client_->sync_computeSumNoCoro(response, request);
    return response.sum;
  });
}

TEST_F(CoroutineTest, Sum) {
  expectSumResults([&](int x, int y) {
    SumRequest request;
    request.x = x;
    request.y = y;
    SumResponse response;
    client_->sync_computeSum(response, request);
    return response.sum;
  });
}

TEST_F(CoroutineTest, SumPrimitive) {
  expectSumResults(
      [&](int x, int y) { return client_->sync_computeSumPrimitive(x, y); });
}

TEST_F(CoroutineTest, SumVoid) {
  expectSumResults([&](int x, int y) {
    client_->sync_computeSumVoid(x, y);
    return voidReturnValue;
  });
}

TEST_F(CoroutineTest, SumEb) {
  expectSumResults([&](int x, int y) {
    SumRequest request;
    request.x = x;
    request.y = y;
    SumResponse response;
    client_->sync_computeSumEb(response, request);
    return response.sum;
  });
}

TEST_F(CoroutineTest, SumUnimplemented) {
  for (int i = 0; i < 10; ++i) {
    bool error = false;
    try {
      SumRequest request;
      request.x = i;
      request.y = i;
      SumResponse response;
      client_->sync_computeSumUnimplemented(response, request);
    } catch (...) {
      error = true;
    }
    EXPECT_TRUE(error);
  }
  expectSumResults(
      [&](int x, int y) { return client_->sync_computeSumPrimitive(x, y); });
}

TEST_F(CoroutineTest, SumUnimplementedPrimitive) {
  for (int i = 0; i < 10; ++i) {
    bool error = false;
    try {
      client_->sync_computeSumUnimplementedPrimitive(i, i);
    } catch (...) {
      error = true;
    }
    EXPECT_TRUE(error);
  }
  expectSumResults(
      [&](int x, int y) { return client_->sync_computeSumPrimitive(x, y); });
}

TEST_F(CoroutineTest, SumThrows) {
  for (int i = 0; i < 10; ++i) {
    bool error = false;
    try {
      SumRequest request;
      request.x = i;
      request.y = i;
      SumResponse response;
      client_->sync_computeSumThrows(response, request);
    } catch (...) {
      error = true;
    }
    EXPECT_TRUE(error);
  }
  expectSumResults(
      [&](int x, int y) { return client_->sync_computeSumPrimitive(x, y); });
}

TEST_F(CoroutineTest, SumThrowsPrimitive) {
  for (int i = 0; i < 10; ++i) {
    bool error = false;
    try {
      client_->sync_computeSumThrowsPrimitive(i, i);
    } catch (...) {
      error = true;
    }
    EXPECT_TRUE(error);
  }
  expectSumResults(
      [&](int x, int y) { return client_->sync_computeSumPrimitive(x, y); });
}

TEST_F(CoroutineTest, NoParameters) {
  EXPECT_EQ(kNoParameterReturnValue, client_->sync_noParameters());
  EXPECT_EQ(kNoParameterReturnValue, client_->sync_noParameters());
  EXPECT_EQ(kNoParameterReturnValue, client_->sync_noParameters());
}

TEST_F(CoroutineTest, ImplemetedWithFutures) {
  SumResponse response;
  response.sum = 0;
  client_->sync_implementedWithFutures(response);
  EXPECT_EQ(kNoParameterReturnValue, response.sum);

  response.sum = 0;
  client_->sync_implementedWithFutures(response);
  EXPECT_EQ(kNoParameterReturnValue, response.sum);

  response.sum = 0;
  client_->sync_implementedWithFutures(response);
  EXPECT_EQ(kNoParameterReturnValue, response.sum);
}

TEST_F(CoroutineTest, ImplemetedWithFuturesPrimitive) {
  EXPECT_EQ(
      kNoParameterReturnValue, client_->sync_implementedWithFuturesPrimitive());
  EXPECT_EQ(
      kNoParameterReturnValue, client_->sync_implementedWithFuturesPrimitive());
  EXPECT_EQ(
      kNoParameterReturnValue, client_->sync_implementedWithFuturesPrimitive());
}

class CoroutineNullTest : public testing::Test {
 public:
  CoroutineNullTest()
      : ssit_(std::make_shared<CoroutineSvNull>()),
        client_(ssit_.newClient<CoroutineAsyncClient>(
            *EventBaseManager::get()->getEventBase())) {}
  ScopedServerInterfaceThread ssit_;
  std::unique_ptr<CoroutineAsyncClient> client_;
};

TEST_F(CoroutineNullTest, Basics) {
  SumRequest request;
  request.x = 123;
  request.y = 123;

  SumResponse response;

  response.sum = 123;
  client_->sync_computeSumNoCoro(response, request);
  EXPECT_EQ(0, response.sum);

  response.sum = 123;
  client_->sync_computeSum(response, request);
  EXPECT_EQ(0, response.sum);

  EXPECT_EQ(0, client_->sync_computeSumPrimitive(123, 456));

  client_->sync_computeSumVoid(123, 456);

  client_->sync_noParameters();
}
