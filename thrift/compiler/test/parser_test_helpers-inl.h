/*
 * Copyright 2016 Facebook, Inc.
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

#include <memory>
#include <string>
#include <vector>

template <typename> struct func_signature_helper;

template <typename ArgType>
class func_signature_helper {
 public:
  static std::unique_ptr<t_type> get_type() {
    return get_type(dummy<typename std::decay<ArgType>::type>());
  }

 private:
  static std::unique_ptr<t_type> get_base_type(
      std::string name,
      t_base_type::t_base type) {
    return std::unique_ptr<t_base_type>(new t_base_type(std::move(name), type));
  }

  template <typename> struct dummy {};
  static std::unique_ptr<t_type> get_type(dummy<void>) {
    return get_base_type("void", t_base_type::TYPE_VOID);
  }
  static std::unique_ptr<t_type> get_type(dummy<int>) {
    return get_base_type("i32", t_base_type::TYPE_I32);
  }
  static std::unique_ptr<t_type> get_type(dummy<double>) {
    return get_base_type("double", t_base_type::TYPE_DOUBLE);
  }
};

template <typename ...> struct func_signature;

template <>
class func_signature<> {
public:
  static void get_arg_type(std::vector<std::unique_ptr<t_type>>& vec) {}
};

template <typename ArgType, typename... Tail>
class func_signature<ArgType, Tail...> {
public:
  static void get_arg_type(std::vector<std::unique_ptr<t_type>>& vec) {
    vec.push_back(func_signature_helper<ArgType>::get_type());
    func_signature<Tail...>::get_arg_type(vec);
  }
};

template <typename ReturnType, typename... ArgsType>
class func_signature<ReturnType(ArgsType...)> {
public:
  static std::unique_ptr<t_type> return_type() {
    return func_signature_helper<ReturnType>::get_type();
  }

  static std::vector<std::unique_ptr<t_type>> args_types() {
    std::vector<std::unique_ptr<t_type>> result;
    func_signature<ArgsType...>::get_arg_type(result);
    return result;
  }
};
