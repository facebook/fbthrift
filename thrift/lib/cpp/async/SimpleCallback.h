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
#ifndef _THRIFT_ASYNC_SIMPLECALLBACK_H_
#define _THRIFT_ASYNC_SIMPLECALLBACK_H_ 1

#include "thrift/lib/cpp/Thrift.h"
namespace apache { namespace thrift {

/**
 * A template class for forming simple method callbacks with either an empty
 * argument list or one argument of known type.
 *
 * For more efficiency where std::function is overkill.
 */

template<typename C,              ///< class whose method we wish to wrap
         typename A = void,       ///< type of argument
         typename R = void>       ///< type of return value
class SimpleCallback {
  typedef R (C::*cfptr_t)(A);     ///< pointer-to-member-function type
  cfptr_t fptr_;                  ///< the embedded function pointer
  C* obj_;                        ///< object whose function we're wrapping
 public:
  /**
   * Constructor for empty callback object.
   */
  SimpleCallback() :
    fptr_(nullptr), obj_(nullptr) {}
  /**
   * Construct callback wrapper for member function.
   *
   * @param fptr pointer-to-member-function
   * @param "this" for object associated with callback
   */
  SimpleCallback(cfptr_t fptr, const C* obj) :
    fptr_(fptr), obj_(const_cast<C*>(obj))
  {}

  /**
   * Make a call to the member function we've wrapped.
   *
   * @param i argument for the wrapped member function
   * @return value from that function
   */
  R operator()(A i) const {
    (obj_->*fptr_)(i);
  }

  explicit operator bool() const {
    return obj_ != nullptr && fptr_ != nullptr;
  }

  ~SimpleCallback() {}
};

/**
 * Specialization of SimpleCallback for empty argument list.
 */
template<typename C,              ///< class whose method we wish to wrap
         typename R>              ///< type of return value
class SimpleCallback<C, void, R> {
  typedef R (C::*cfptr_t)();      ///< pointer-to-member-function type
  cfptr_t fptr_;                  ///< the embedded function pointer
  C* obj_;                        ///< object whose function we're wrapping
 public:
  /**
   * Constructor for empty callback object.
   */
  SimpleCallback() :
    fptr_(nullptr), obj_(nullptr) {}

  /**
   * Construct callback wrapper for member function.
   *
   * @param fptr pointer-to-member-function
   * @param obj "this" for object associated with callback
   */
  SimpleCallback(cfptr_t fptr, const C* obj) :
    fptr_(fptr), obj_(const_cast<C*>(obj))
  {}

  /**
   * Make a call to the member function we've wrapped.
   *
   * @return value from that function
   */
  R operator()() const {
    (obj_->*fptr_)();
  }

  explicit operator bool() const {
    return obj_ != nullptr && fptr_ != nullptr;
  }

  ~SimpleCallback() {}
};

}} // apache::thrift

#endif /* !_THRIFT_ASYNC_SIMPLECALLBACK_H_ */
