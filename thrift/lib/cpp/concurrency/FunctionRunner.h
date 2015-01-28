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

#ifndef _THRIFT_CONCURRENCY_FUNCTION_RUNNER_H
#define _THRIFT_CONCURRENCY_FUNCTION_RUNNER_H 1

#include <unistd.h>
#include <functional>
#include <thrift/lib/cpp/concurrency/Thread.h>

namespace apache { namespace thrift { namespace concurrency {

/**
 * Convenient implementation of Runnable that will execute arbitrary callbacks.
 * Interfaces are provided to accept both a generic 'void(void)' callback, and
 * a 'void* (void*)' pthread_create-style callback.
 *
 * Example use:
 *  void* my_thread_main(void* arg);
 *  shared_ptr<ThreadFactory> factory = ...;
 *  // To create a thread that executes my_thread_main once:
 *  shared_ptr<Thread> thread = factory->newThread(
 *    FunctionRunner::create(my_thread_main, some_argument));
 *  thread->start();
 *
 *  bool A::foo();
 *  A* a = new A();
 *  // To create a thread that executes a.foo() every 100 milliseconds:
 *  factory->newThread(FunctionRunner::create(
 *    std::bind(&A::foo, a), 100))->start();
 *
 */

class FunctionRunner : public Runnable {
 public:
  // This is the type of callback 'pthread_create()' expects.
  typedef void* (*PthreadFuncPtr)(void *arg);
  // This a fully-generic void(void) callback for custom bindings.
  typedef std::function<void()> VoidFunc;

  typedef std::function<bool()> BoolFunc;

  /**
   * Syntactic sugar to make it easier to create new FunctionRunner
   * objects wrapped in shared_ptr.
   */
  template <class F>
  static std::shared_ptr<FunctionRunner> create(F&& cob) {
    return std::shared_ptr<FunctionRunner>(
      new FunctionRunner(std::forward<F>(cob)));
  }

  static std::shared_ptr<FunctionRunner> create(const PthreadFuncPtr& func,
                                                void* arg) {
    return std::shared_ptr<FunctionRunner>(new FunctionRunner(func, arg));
  }

  template <class F>
  static std::shared_ptr<FunctionRunner> create(F&& cob,
                                                int intervalMs) {
    return std::shared_ptr<FunctionRunner>(
      new FunctionRunner(std::forward<F>(cob), intervalMs));
  }

  /**
   * Given a 'pthread_create' style callback, this FunctionRunner will
   * execute the given callback.  Note that the 'void*' return value is ignored.
   */
  FunctionRunner(PthreadFuncPtr func, void* arg)
   : func_(std::bind(func, arg)), repFunc_(0), initFunc_(0)
  { }

  /**
   * Given a generic callback, this FunctionRunner will execute it.
   */
  template <class F>
  explicit FunctionRunner(F&& cob)
   : func_(std::forward<F>(cob)), repFunc_(0), initFunc_(0)
  { }

  /**
   * Given a bool foo(...) type callback, FunctionRunner will execute
   * the callback repeatedly with 'intervalMs' milliseconds between the calls,
   * until it returns false. Note that the actual interval between calls will
   * be intervalMs plus execution time of the callback.
   */
  template <class F>
  FunctionRunner(F&& cob, int intervalMs)
   : func_(0), repFunc_(std::forward<F>(cob)), intervalMs_(intervalMs),
     initFunc_(0)
  { }

  /**
   * Set a callback to be called when the thread is started.
   */
  void setInitFunc(const VoidFunc& initFunc) {
    initFunc_ = initFunc;
  }

  void run() {
    if (initFunc_) {
      initFunc_();
    }
    if (repFunc_) {
      while(repFunc_()) {
        usleep(intervalMs_*1000);
      }
    } else {
      func_();
    }
  }

 private:
  VoidFunc func_;
  BoolFunc repFunc_;
  int intervalMs_;
  VoidFunc initFunc_;
};

}}} // apache::thrift::concurrency

#endif // #ifndef _THRIFT_CONCURRENCY_FUNCTION_RUNNER_H
