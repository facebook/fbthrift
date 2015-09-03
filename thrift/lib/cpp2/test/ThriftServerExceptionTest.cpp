/*
 * Copyright 2015 Facebook, Inc.
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

#include <thrift/lib/cpp2/test/gen-cpp2/Raiser.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

#include <gtest/gtest.h>

using namespace std;
using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::test;

class lulz : public exception {
public:
  explicit lulz(string message) noexcept : message_(move(message)) {}
  const char* what() const noexcept override { return message_.c_str(); }
private:
  string message_;
};

namespace {

using AppExn = TApplicationException;

class RaiserHandler : public RaiserSvIf {
public:
  explicit RaiserHandler(function<exception_ptr()> go) :
    go_(wrap(move(go))) {}
  explicit RaiserHandler(function<exception_wrapper()> go) :
    go_(wrap(move(go))) {}

protected:
  void async_tm_doBland(unique_ptr<HandlerCallback<void>> cb) override {
    go_(move(cb));
  }
  void async_tm_doRaise(unique_ptr<HandlerCallback<void>> cb) override {
    go_(move(cb));
  }
  void async_tm_get200(unique_ptr<HandlerCallback<string>> cb) override {
    go_(move(cb));
  }
  void async_tm_get500(unique_ptr<HandlerCallback<string>> cb) override {
    go_(move(cb));
  }

  template <typename E>
  function<void(unique_ptr<HandlerCallbackBase>)> wrap(E e) {
    auto em = makeMoveWrapper(move(e));
    return [=](unique_ptr<HandlerCallbackBase> cb) { cb->exception((*em)()); };
  }

private:
  function<void(unique_ptr<HandlerCallbackBase>)> go_;
};

}

class ThriftServerExceptionTest : public testing::Test {
public:
  EventBase eb;

  string message { "rofl" };

  template <class E> exception_ptr to_eptr(const E& e) {
    try { throw e; }
    catch (E&) { return current_exception(); }
  }

  template <class E> exception_wrapper to_wrap(const E& e) {
    return exception_wrapper(e);  // just an alias
  }

  lulz make_lulz() const { return lulz(message); }
  Banal make_banal() const { return Banal(); }
  Fiery make_fiery() const { Fiery f; f.message = message; return f; }

  template <typename T>
  struct action_traits_impl;
  template <typename C, typename A>
  struct action_traits_impl<void(C::*)(A&) const> { using arg_type = A; };
  template <typename C, typename A>
  struct action_traits_impl<void(C::*)(A&)> { using arg_type = A; };
  template <typename F>
  using action_traits = action_traits_impl<decltype(&F::operator())>;
  template <typename F>
  using arg = typename action_traits<F>::arg_type;

  template <class V, class F>
  bool exn(Future<V> fv, F&& f) {
    using E = typename std::decay<arg<F>>::type;
    exception_wrapper wrap = fv.waitVia(&eb).getTry().exception();
    return wrap.with_exception<E>(move(f));
  }
};

TEST_F(ThriftServerExceptionTest, bland_with_exception_ptr) {
  auto go = [&] { return to_eptr(make_lulz()); };

  auto handler = make_shared<RaiserHandler>(go);
  ScopedServerInterfaceThread runner(handler);

  auto client = runner.newClient<RaiserAsyncClient>(&eb);

  auto lulz_w = sformat("lulz: {}", message);

  EXPECT_TRUE(exn(client->future_doBland(), [&](const AppExn& e) {
      EXPECT_EQ(AppExn::TApplicationExceptionType::UNKNOWN, e.getType());
      EXPECT_EQ(lulz_w, string(e.what()));
  }));
  EXPECT_TRUE(exn(client->future_doRaise(), [&](const AppExn& e) {
      EXPECT_EQ(AppExn::TApplicationExceptionType::UNKNOWN, e.getType());
      EXPECT_EQ(lulz_w, string(e.what()));
  }));
  EXPECT_TRUE(exn(client->future_get200(), [&](const AppExn& e) {
      EXPECT_EQ(AppExn::TApplicationExceptionType::UNKNOWN, e.getType());
      EXPECT_EQ(lulz_w, string(e.what()));
  }));
  EXPECT_TRUE(exn(client->future_get500(), [&](const AppExn& e) {
      EXPECT_EQ(AppExn::TApplicationExceptionType::UNKNOWN, e.getType());
      EXPECT_EQ(lulz_w, string(e.what()));
  }));
}

TEST_F(ThriftServerExceptionTest, banal_with_exception_ptr) {
  auto go = [&] { return to_eptr(make_banal()); };

  auto handler = make_shared<RaiserHandler>(go);
  ScopedServerInterfaceThread runner(handler);

  auto client = runner.newClient<RaiserAsyncClient>(&eb);

  auto banal_s = string{"apache::thrift::test::Banal"};
  auto banal_w_guess = sformat("{0}:  ::{0}", banal_s);
  auto banal_w_known = sformat(" ::{0}", banal_s);

  EXPECT_TRUE(exn(client->future_doBland(), [&](const AppExn& e) {
      EXPECT_EQ(banal_w_guess, string(e.what()));
  }));
  EXPECT_TRUE(exn(client->future_doRaise(), [&](const Banal& e) {
      EXPECT_EQ(banal_w_known, string(e.what()));
  }));
  EXPECT_TRUE(exn(client->future_get200(), [&](const AppExn& e) {
      EXPECT_EQ(banal_w_guess, string(e.what()));
  }));
  EXPECT_TRUE(exn(client->future_get500(), [&](const Banal& e) {
      EXPECT_EQ(banal_w_known, string(e.what()));
  }));
}

TEST_F(ThriftServerExceptionTest, fiery_with_exception_ptr) {
  auto go = [&] { return to_eptr(make_fiery()); };

  auto handler = make_shared<RaiserHandler>(go);
  ScopedServerInterfaceThread runner(handler);

  auto client = runner.newClient<RaiserAsyncClient>(&eb);

  auto fiery_s = string{"apache::thrift::test::Fiery"};
  auto fiery_w_guess = sformat("{0}:  ::{0}", fiery_s);
  auto fiery_w_known = sformat(" ::{0}", fiery_s);

  EXPECT_TRUE(exn(client->future_doBland(), [&](const AppExn& e) {
      EXPECT_EQ(fiery_w_guess, string(e.what()));
  }));
  EXPECT_TRUE(exn(client->future_doRaise(), [&](const Fiery& e) {
      EXPECT_EQ(fiery_w_known, string(e.what()));
      EXPECT_EQ(message, e.message);
  }));
  EXPECT_TRUE(exn(client->future_get200(), [&](const AppExn& e) {
      EXPECT_EQ(fiery_w_guess, string(e.what()));
  }));
  EXPECT_TRUE(exn(client->future_get500(), [&](const Fiery& e) {
      EXPECT_EQ(fiery_w_known, string(e.what()));
      EXPECT_EQ(message, e.message);
  }));
}

TEST_F(ThriftServerExceptionTest, bland_with_exception_wrapper) {
  auto go = [&] { return to_wrap(make_lulz()); };

  auto handler = make_shared<RaiserHandler>(go);
  ScopedServerInterfaceThread runner(handler);

  auto client = runner.newClient<RaiserAsyncClient>(&eb);

  auto lulz_w = sformat("lulz: {}", message);

  EXPECT_TRUE(exn(client->future_doBland(), [&](const AppExn& e) {
      EXPECT_EQ(AppExn::TApplicationExceptionType::UNKNOWN, e.getType());
      EXPECT_EQ(lulz_w, string(e.what()));
  }));
  EXPECT_TRUE(exn(client->future_doRaise(), [&](const AppExn& e) {
      EXPECT_EQ(AppExn::TApplicationExceptionType::UNKNOWN, e.getType());
      EXPECT_EQ(lulz_w, string(e.what()));
  }));
  EXPECT_TRUE(exn(client->future_get200(), [&](const AppExn& e) {
      EXPECT_EQ(AppExn::TApplicationExceptionType::UNKNOWN, e.getType());
      EXPECT_EQ(lulz_w, string(e.what()));
  }));
  EXPECT_TRUE(exn(client->future_get500(), [&](const AppExn& e) {
      EXPECT_EQ(AppExn::TApplicationExceptionType::UNKNOWN, e.getType());
      EXPECT_EQ(lulz_w, string(e.what()));
  }));
}

TEST_F(ThriftServerExceptionTest, banal_with_exception_wrapper) {
  auto go = [&] { return to_wrap(make_banal()); };

  auto handler = make_shared<RaiserHandler>(go);
  ScopedServerInterfaceThread runner(handler);

  auto client = runner.newClient<RaiserAsyncClient>(&eb);

  auto banal_s = string{"apache::thrift::test::Banal"};
  auto banal_w_guess = sformat("{0}:  ::{0}", banal_s);
  auto banal_w_known = sformat(" ::{0}", banal_s);

  EXPECT_TRUE(exn(client->future_doBland(), [&](const AppExn& e) {
      EXPECT_EQ(banal_w_guess, string(e.what()));
  }));
  EXPECT_TRUE(exn(client->future_doRaise(), [&](const Banal& e) {
      EXPECT_EQ(banal_w_known, string(e.what()));
  }));
  EXPECT_TRUE(exn(client->future_get200(), [&](const AppExn& e) {
      EXPECT_EQ(banal_w_guess, string(e.what()));
  }));
  EXPECT_TRUE(exn(client->future_get500(), [&](const Banal& e) {
      EXPECT_EQ(banal_w_known, string(e.what()));
  }));
}

TEST_F(ThriftServerExceptionTest, fiery_with_exception_wrapper) {
  auto go = [&] { return to_wrap(make_fiery()); };

  auto handler = make_shared<RaiserHandler>(go);
  ScopedServerInterfaceThread runner(handler);

  auto client = runner.newClient<RaiserAsyncClient>(&eb);

  auto fiery_s = string{"apache::thrift::test::Fiery"};
  auto fiery_w_guess = sformat("{0}:  ::{0}", fiery_s);
  auto fiery_w_known = sformat(" ::{0}", fiery_s);

  EXPECT_TRUE(exn(client->future_doBland(), [&](const AppExn& e) {
      EXPECT_EQ(fiery_w_guess, string(e.what()));
  }));
  EXPECT_TRUE(exn(client->future_doRaise(), [&](const Fiery& e) {
      EXPECT_EQ(fiery_w_known, string(e.what()));
      EXPECT_EQ(message, e.message);
  }));
  EXPECT_TRUE(exn(client->future_get200(), [&](const AppExn& e) {
      EXPECT_EQ(fiery_w_guess, string(e.what()));
  }));
  EXPECT_TRUE(exn(client->future_get500(), [&](const Fiery& e) {
      EXPECT_EQ(fiery_w_known, string(e.what()));
      EXPECT_EQ(message, e.message);
  }));
}
