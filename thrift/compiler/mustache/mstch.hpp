#pragma once

#include <vector>
#include <map>
#include <string>
#include <memory>
#include <functional>

#include <boost/variant.hpp>

namespace mstch {

struct config {
  static std::function<std::string(const std::string&)> escape;
};

namespace internal {

template<class N>
class object_t {
 public:
  const N& at(const std::string& name) const {
    cache[name] = (methods.at(name))();
    return cache[name];
  }

  bool has(const std::string name) const {
    return methods.count(name) != 0;
  }

 protected:
  template<class S>
  void register_methods(S* s, std::map<std::string,N(S::*)()> methods) {
    for(auto& item: methods)
      this->methods.insert({item.first, std::bind(item.second, s)});
  }

 private:
  std::map<std::string, std::function<N()>> methods;
  mutable std::map<std::string, N> cache;
};

template<class T, class N>
class is_fun {
 private:
  using not_fun = char;
  using fun_without_args = char[2];
  using fun_with_args = char[3];
  template <typename U, U> struct really_has;
  template <typename C> static fun_without_args& test(
      really_has<N(C::*)() const, &C::operator()>*);
  template <typename C> static fun_with_args& test(
      really_has<N(C::*)(const std::string&) const,
      &C::operator()>*);
  template <typename> static not_fun& test(...);

 public:
  static bool const no_args = sizeof(test<T>(0)) == sizeof(fun_without_args);
  static bool const has_args = sizeof(test<T>(0)) == sizeof(fun_with_args);
};

template<class N>
using node_renderer = std::function<std::string(const N& n)>;

template<class N>
class lambda_t {
 public:
  template<class F>
  lambda_t(F f, typename std::enable_if<is_fun<F, N>::no_args>::type* = 0):
      fun([f](node_renderer<N> renderer, const std::string&) {
        return renderer(f());
      })
  {
  }

  template<class F>
  lambda_t(F f, typename std::enable_if<is_fun<F, N>::has_args>::type* = 0):
      fun([f](node_renderer<N> renderer, const std::string& text) {
        return renderer(f(text));
      })
  {
  }

  std::string operator()(node_renderer<N> renderer,
      const std::string& text = "") const
  {
    return fun(renderer, text);
  }

 private:
  std::function<std::string(node_renderer<N> renderer, const std::string&)> fun;
};

}

using node = boost::make_recursive_variant<
    std::nullptr_t, std::string, int, double, bool,
    internal::lambda_t<boost::recursive_variant_>,
    std::shared_ptr<internal::object_t<boost::recursive_variant_>>,
    std::map<const std::string, boost::recursive_variant_>,
    std::vector<boost::recursive_variant_>>::type;
using object = internal::object_t<node>;
using lambda = internal::lambda_t<node>;
using map = std::map<const std::string, node>;
using array = std::vector<node>;

std::string render(
    const std::string& tmplt,
    const node& root,
    const std::map<std::string,std::string>& partials =
        std::map<std::string,std::string>());

}
