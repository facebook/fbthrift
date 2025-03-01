/**
 * Autogenerated by Thrift for thrift/compiler/test/fixtures/py3/src/module.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */

#include <thrift/compiler/test/fixtures/py3/gen-py3/module/services_wrapper.h>
#include <thrift/compiler/test/fixtures/py3/gen-py3/module/services_api.h>
#include <thrift/lib/cpp2/async/AsyncProcessor.h>

namespace py3 {
namespace simple {

SimpleServiceWrapper::SimpleServiceWrapper(PyObject *obj, folly::Executor* exc)
  : if_object(obj), executor(exc)
  {
    import_module__services();
  }


void SimpleServiceWrapper::async_tm_get_five(
  apache::thrift::HandlerCallbackPtr<int32_t> callback) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback)
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<int32_t>();
        call_cy_SimpleService_get_five(
            this->if_object,
            ctx,
            std::move(promise)        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<int32_t>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_add_five(
  apache::thrift::HandlerCallbackPtr<int32_t> callback
    , int32_t num
) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback),
     num
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<int32_t>();
        call_cy_SimpleService_add_five(
            this->if_object,
            ctx,
            std::move(promise),
            num        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<int32_t>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_do_nothing(
  apache::thrift::HandlerCallbackPtr<void> callback) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback)
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<folly::Unit>();
        call_cy_SimpleService_do_nothing(
            this->if_object,
            ctx,
            std::move(promise)        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<folly::Unit>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_concat(
  apache::thrift::HandlerCallbackPtr<std::unique_ptr<std::string>> callback
    , std::unique_ptr<std::string> first
    , std::unique_ptr<std::string> second
) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback),
     first = std::move(first),
     second = std::move(second)
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<std::unique_ptr<std::string>>();
        call_cy_SimpleService_concat(
            this->if_object,
            ctx,
            std::move(promise),
            std::move(first),
            std::move(second)        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<std::unique_ptr<std::string>>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_get_value(
  apache::thrift::HandlerCallbackPtr<int32_t> callback
    , std::unique_ptr<::py3::simple::SimpleStruct> simple_struct
) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback),
     simple_struct = std::move(simple_struct)
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<int32_t>();
        call_cy_SimpleService_get_value(
            this->if_object,
            ctx,
            std::move(promise),
            std::move(simple_struct)        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<int32_t>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_negate(
  apache::thrift::HandlerCallbackPtr<bool> callback
    , bool input
) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback),
     input
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<bool>();
        call_cy_SimpleService_negate(
            this->if_object,
            ctx,
            std::move(promise),
            input        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<bool>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_tiny(
  apache::thrift::HandlerCallbackPtr<int8_t> callback
    , int8_t input
) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback),
     input
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<int8_t>();
        call_cy_SimpleService_tiny(
            this->if_object,
            ctx,
            std::move(promise),
            input        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<int8_t>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_small(
  apache::thrift::HandlerCallbackPtr<int16_t> callback
    , int16_t input
) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback),
     input
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<int16_t>();
        call_cy_SimpleService_small(
            this->if_object,
            ctx,
            std::move(promise),
            input        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<int16_t>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_big(
  apache::thrift::HandlerCallbackPtr<int64_t> callback
    , int64_t input
) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback),
     input
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<int64_t>();
        call_cy_SimpleService_big(
            this->if_object,
            ctx,
            std::move(promise),
            input        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<int64_t>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_two(
  apache::thrift::HandlerCallbackPtr<double> callback
    , double input
) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback),
     input
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<double>();
        call_cy_SimpleService_two(
            this->if_object,
            ctx,
            std::move(promise),
            input        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<double>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_expected_exception(
  apache::thrift::HandlerCallbackPtr<void> callback) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback)
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<folly::Unit>();
        call_cy_SimpleService_expected_exception(
            this->if_object,
            ctx,
            std::move(promise)        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<folly::Unit>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_unexpected_exception(
  apache::thrift::HandlerCallbackPtr<int32_t> callback) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback)
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<int32_t>();
        call_cy_SimpleService_unexpected_exception(
            this->if_object,
            ctx,
            std::move(promise)        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<int32_t>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_sum_i16_list(
  apache::thrift::HandlerCallbackPtr<int32_t> callback
    , std::unique_ptr<std::vector<int16_t>> numbers
) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback),
     numbers = std::move(numbers)
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<int32_t>();
        call_cy_SimpleService_sum_i16_list(
            this->if_object,
            ctx,
            std::move(promise),
            std::move(numbers)        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<int32_t>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_sum_i32_list(
  apache::thrift::HandlerCallbackPtr<int32_t> callback
    , std::unique_ptr<std::vector<int32_t>> numbers
) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback),
     numbers = std::move(numbers)
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<int32_t>();
        call_cy_SimpleService_sum_i32_list(
            this->if_object,
            ctx,
            std::move(promise),
            std::move(numbers)        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<int32_t>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_sum_i64_list(
  apache::thrift::HandlerCallbackPtr<int32_t> callback
    , std::unique_ptr<std::vector<int64_t>> numbers
) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback),
     numbers = std::move(numbers)
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<int32_t>();
        call_cy_SimpleService_sum_i64_list(
            this->if_object,
            ctx,
            std::move(promise),
            std::move(numbers)        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<int32_t>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_concat_many(
  apache::thrift::HandlerCallbackPtr<std::unique_ptr<std::string>> callback
    , std::unique_ptr<std::vector<std::string>> words
) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback),
     words = std::move(words)
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<std::unique_ptr<std::string>>();
        call_cy_SimpleService_concat_many(
            this->if_object,
            ctx,
            std::move(promise),
            std::move(words)        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<std::unique_ptr<std::string>>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_count_structs(
  apache::thrift::HandlerCallbackPtr<int32_t> callback
    , std::unique_ptr<std::vector<::py3::simple::SimpleStruct>> items
) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback),
     items = std::move(items)
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<int32_t>();
        call_cy_SimpleService_count_structs(
            this->if_object,
            ctx,
            std::move(promise),
            std::move(items)        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<int32_t>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_sum_set(
  apache::thrift::HandlerCallbackPtr<int32_t> callback
    , std::unique_ptr<std::set<int32_t>> numbers
) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback),
     numbers = std::move(numbers)
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<int32_t>();
        call_cy_SimpleService_sum_set(
            this->if_object,
            ctx,
            std::move(promise),
            std::move(numbers)        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<int32_t>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_contains_word(
  apache::thrift::HandlerCallbackPtr<bool> callback
    , std::unique_ptr<std::set<std::string>> words
    , std::unique_ptr<std::string> word
) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback),
     words = std::move(words),
     word = std::move(word)
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<bool>();
        call_cy_SimpleService_contains_word(
            this->if_object,
            ctx,
            std::move(promise),
            std::move(words),
            std::move(word)        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<bool>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_get_map_value(
  apache::thrift::HandlerCallbackPtr<std::unique_ptr<std::string>> callback
    , std::unique_ptr<std::map<std::string,std::string>> words
    , std::unique_ptr<std::string> key
) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback),
     words = std::move(words),
     key = std::move(key)
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<std::unique_ptr<std::string>>();
        call_cy_SimpleService_get_map_value(
            this->if_object,
            ctx,
            std::move(promise),
            std::move(words),
            std::move(key)        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<std::unique_ptr<std::string>>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_map_length(
  apache::thrift::HandlerCallbackPtr<int16_t> callback
    , std::unique_ptr<std::map<std::string,::py3::simple::SimpleStruct>> items
) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback),
     items = std::move(items)
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<int16_t>();
        call_cy_SimpleService_map_length(
            this->if_object,
            ctx,
            std::move(promise),
            std::move(items)        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<int16_t>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_sum_map_values(
  apache::thrift::HandlerCallbackPtr<int16_t> callback
    , std::unique_ptr<std::map<std::string,int16_t>> items
) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback),
     items = std::move(items)
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<int16_t>();
        call_cy_SimpleService_sum_map_values(
            this->if_object,
            ctx,
            std::move(promise),
            std::move(items)        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<int16_t>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_complex_sum_i32(
  apache::thrift::HandlerCallbackPtr<int32_t> callback
    , std::unique_ptr<::py3::simple::ComplexStruct> counter
) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback),
     counter = std::move(counter)
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<int32_t>();
        call_cy_SimpleService_complex_sum_i32(
            this->if_object,
            ctx,
            std::move(promise),
            std::move(counter)        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<int32_t>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_repeat_name(
  apache::thrift::HandlerCallbackPtr<std::unique_ptr<std::string>> callback
    , std::unique_ptr<::py3::simple::ComplexStruct> counter
) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback),
     counter = std::move(counter)
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<std::unique_ptr<std::string>>();
        call_cy_SimpleService_repeat_name(
            this->if_object,
            ctx,
            std::move(promise),
            std::move(counter)        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<std::unique_ptr<std::string>>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_get_struct(
  apache::thrift::HandlerCallbackPtr<std::unique_ptr<::py3::simple::SimpleStruct>> callback) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback)
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<std::unique_ptr<::py3::simple::SimpleStruct>>();
        call_cy_SimpleService_get_struct(
            this->if_object,
            ctx,
            std::move(promise)        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<std::unique_ptr<::py3::simple::SimpleStruct>>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_fib(
  apache::thrift::HandlerCallbackPtr<std::unique_ptr<std::vector<int32_t>>> callback
    , int16_t n
) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback),
     n
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<std::unique_ptr<std::vector<int32_t>>>();
        call_cy_SimpleService_fib(
            this->if_object,
            ctx,
            std::move(promise),
            n        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<std::unique_ptr<std::vector<int32_t>>>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_unique_words(
  apache::thrift::HandlerCallbackPtr<std::unique_ptr<std::set<std::string>>> callback
    , std::unique_ptr<std::vector<std::string>> words
) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback),
     words = std::move(words)
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<std::unique_ptr<std::set<std::string>>>();
        call_cy_SimpleService_unique_words(
            this->if_object,
            ctx,
            std::move(promise),
            std::move(words)        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<std::unique_ptr<std::set<std::string>>>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_words_count(
  apache::thrift::HandlerCallbackPtr<std::unique_ptr<std::map<std::string,int16_t>>> callback
    , std::unique_ptr<std::vector<std::string>> words
) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback),
     words = std::move(words)
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<std::unique_ptr<std::map<std::string,int16_t>>>();
        call_cy_SimpleService_words_count(
            this->if_object,
            ctx,
            std::move(promise),
            std::move(words)        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<std::unique_ptr<std::map<std::string,int16_t>>>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_set_enum(
  apache::thrift::HandlerCallbackPtr<::py3::simple::AnEnum> callback
    , ::py3::simple::AnEnum in_enum
) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback),
     in_enum
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<::py3::simple::AnEnum>();
        call_cy_SimpleService_set_enum(
            this->if_object,
            ctx,
            std::move(promise),
            in_enum        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<::py3::simple::AnEnum>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_list_of_lists(
  apache::thrift::HandlerCallbackPtr<std::unique_ptr<std::vector<std::vector<int32_t>>>> callback
    , int16_t num_lists
    , int16_t num_items
) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback),
     num_lists,
     num_items
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<std::unique_ptr<std::vector<std::vector<int32_t>>>>();
        call_cy_SimpleService_list_of_lists(
            this->if_object,
            ctx,
            std::move(promise),
            num_lists,
            num_items        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<std::unique_ptr<std::vector<std::vector<int32_t>>>>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_word_character_frequency(
  apache::thrift::HandlerCallbackPtr<std::unique_ptr<std::map<std::string,std::map<std::string,int32_t>>>> callback
    , std::unique_ptr<std::string> sentence
) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback),
     sentence = std::move(sentence)
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<std::unique_ptr<std::map<std::string,std::map<std::string,int32_t>>>>();
        call_cy_SimpleService_word_character_frequency(
            this->if_object,
            ctx,
            std::move(promise),
            std::move(sentence)        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<std::unique_ptr<std::map<std::string,std::map<std::string,int32_t>>>>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_list_of_sets(
  apache::thrift::HandlerCallbackPtr<std::unique_ptr<std::vector<std::set<std::string>>>> callback
    , std::unique_ptr<std::string> some_words
) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback),
     some_words = std::move(some_words)
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<std::unique_ptr<std::vector<std::set<std::string>>>>();
        call_cy_SimpleService_list_of_sets(
            this->if_object,
            ctx,
            std::move(promise),
            std::move(some_words)        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<std::unique_ptr<std::vector<std::set<std::string>>>>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_nested_map_argument(
  apache::thrift::HandlerCallbackPtr<int32_t> callback
    , std::unique_ptr<std::map<std::string,std::vector<::py3::simple::SimpleStruct>>> struct_map
) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback),
     struct_map = std::move(struct_map)
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<int32_t>();
        call_cy_SimpleService_nested_map_argument(
            this->if_object,
            ctx,
            std::move(promise),
            std::move(struct_map)        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<int32_t>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_make_sentence(
  apache::thrift::HandlerCallbackPtr<std::unique_ptr<std::string>> callback
    , std::unique_ptr<std::vector<std::vector<std::string>>> word_chars
) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback),
     word_chars = std::move(word_chars)
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<std::unique_ptr<std::string>>();
        call_cy_SimpleService_make_sentence(
            this->if_object,
            ctx,
            std::move(promise),
            std::move(word_chars)        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<std::unique_ptr<std::string>>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_get_union(
  apache::thrift::HandlerCallbackPtr<std::unique_ptr<std::set<int32_t>>> callback
    , std::unique_ptr<std::vector<std::set<int32_t>>> sets
) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback),
     sets = std::move(sets)
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<std::unique_ptr<std::set<int32_t>>>();
        call_cy_SimpleService_get_union(
            this->if_object,
            ctx,
            std::move(promise),
            std::move(sets)        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<std::unique_ptr<std::set<int32_t>>>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_get_keys(
  apache::thrift::HandlerCallbackPtr<std::unique_ptr<std::set<std::string>>> callback
    , std::unique_ptr<std::vector<std::map<std::string,std::string>>> string_map
) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback),
     string_map = std::move(string_map)
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<std::unique_ptr<std::set<std::string>>>();
        call_cy_SimpleService_get_keys(
            this->if_object,
            ctx,
            std::move(promise),
            std::move(string_map)        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<std::unique_ptr<std::set<std::string>>>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_lookup_double(
  apache::thrift::HandlerCallbackPtr<double> callback
    , int32_t key
) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback),
     key
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<double>();
        call_cy_SimpleService_lookup_double(
            this->if_object,
            ctx,
            std::move(promise),
            key        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<double>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_retrieve_binary(
  apache::thrift::HandlerCallbackPtr<std::unique_ptr<std::string>> callback
    , std::unique_ptr<std::string> something
) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback),
     something = std::move(something)
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<std::unique_ptr<std::string>>();
        call_cy_SimpleService_retrieve_binary(
            this->if_object,
            ctx,
            std::move(promise),
            std::move(something)        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<std::unique_ptr<std::string>>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_contain_binary(
  apache::thrift::HandlerCallbackPtr<std::unique_ptr<std::set<std::string>>> callback
    , std::unique_ptr<std::vector<std::string>> binaries
) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback),
     binaries = std::move(binaries)
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<std::unique_ptr<std::set<std::string>>>();
        call_cy_SimpleService_contain_binary(
            this->if_object,
            ctx,
            std::move(promise),
            std::move(binaries)        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<std::unique_ptr<std::set<std::string>>>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_contain_enum(
  apache::thrift::HandlerCallbackPtr<std::unique_ptr<std::vector<::py3::simple::AnEnum>>> callback
    , std::unique_ptr<std::vector<::py3::simple::AnEnum>> the_enum
) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback),
     the_enum = std::move(the_enum)
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<std::unique_ptr<std::vector<::py3::simple::AnEnum>>>();
        call_cy_SimpleService_contain_enum(
            this->if_object,
            ctx,
            std::move(promise),
            std::move(the_enum)        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<std::unique_ptr<std::vector<::py3::simple::AnEnum>>>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
void SimpleServiceWrapper::async_tm_get_binary_union_struct(
  apache::thrift::HandlerCallbackPtr<std::unique_ptr<::py3::simple::BinaryUnionStruct>> callback
    , std::unique_ptr<::py3::simple::BinaryUnion> u
) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback),
     u = std::move(u)
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<std::unique_ptr<::py3::simple::BinaryUnionStruct>>();
        call_cy_SimpleService_get_binary_union_struct(
            this->if_object,
            ctx,
            std::move(promise),
            std::move(u)        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<std::unique_ptr<::py3::simple::BinaryUnionStruct>>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
std::shared_ptr<apache::thrift::ServerInterface> SimpleServiceInterface(PyObject *if_object, folly::Executor *exc) {
  return std::make_shared<SimpleServiceWrapper>(if_object, exc);
}
folly::SemiFuture<folly::Unit> SimpleServiceWrapper::semifuture_onStartServing() {
  auto [promise, future] = folly::makePromiseContract<folly::Unit>();
  call_cy_SimpleService_onStartServing(
      this->if_object,
      std::move(promise)
  );
  return std::move(future);
}
folly::SemiFuture<folly::Unit> SimpleServiceWrapper::semifuture_onStopRequested() {
  auto [promise, future] = folly::makePromiseContract<folly::Unit>();
  call_cy_SimpleService_onStopRequested(
      this->if_object,
      std::move(promise)
  );
  return std::move(future);
}


DerivedServiceWrapper::DerivedServiceWrapper(PyObject *obj, folly::Executor* exc)
  : ::py3::simple::SimpleServiceWrapper(obj, exc)
  {
    import_module__services();
  }

void DerivedServiceWrapper::async_tm_get_six(
  apache::thrift::HandlerCallbackPtr<int32_t> callback) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback)
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<int32_t>();
        call_cy_DerivedService_get_six(
            this->if_object,
            ctx,
            std::move(promise)        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<int32_t>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
std::shared_ptr<apache::thrift::ServerInterface> DerivedServiceInterface(PyObject *if_object, folly::Executor *exc) {
  return std::make_shared<DerivedServiceWrapper>(if_object, exc);
}
folly::SemiFuture<folly::Unit> DerivedServiceWrapper::semifuture_onStartServing() {
  auto [promise, future] = folly::makePromiseContract<folly::Unit>();
  call_cy_DerivedService_onStartServing(
      this->if_object,
      std::move(promise)
  );
  return std::move(future);
}
folly::SemiFuture<folly::Unit> DerivedServiceWrapper::semifuture_onStopRequested() {
  auto [promise, future] = folly::makePromiseContract<folly::Unit>();
  call_cy_DerivedService_onStopRequested(
      this->if_object,
      std::move(promise)
  );
  return std::move(future);
}


RederivedServiceWrapper::RederivedServiceWrapper(PyObject *obj, folly::Executor* exc)
  : ::py3::simple::DerivedServiceWrapper(obj, exc)
  {
    import_module__services();
  }

void RederivedServiceWrapper::async_tm_get_seven(
  apache::thrift::HandlerCallbackPtr<int32_t> callback) {
  auto ctx = callback->getRequestContext();
  folly::via(
    this->executor,
    [this, ctx,
     callback = std::move(callback)
    ]() mutable {
        auto [promise, future] = folly::makePromiseContract<int32_t>();
        call_cy_RederivedService_get_seven(
            this->if_object,
            ctx,
            std::move(promise)        );
        std::move(future).via(this->executor).thenTry([callback = std::move(callback)](folly::Try<int32_t>&& t) {
          (void)t;
          callback->complete(std::move(t));
        });
    });
}
std::shared_ptr<apache::thrift::ServerInterface> RederivedServiceInterface(PyObject *if_object, folly::Executor *exc) {
  return std::make_shared<RederivedServiceWrapper>(if_object, exc);
}
folly::SemiFuture<folly::Unit> RederivedServiceWrapper::semifuture_onStartServing() {
  auto [promise, future] = folly::makePromiseContract<folly::Unit>();
  call_cy_RederivedService_onStartServing(
      this->if_object,
      std::move(promise)
  );
  return std::move(future);
}
folly::SemiFuture<folly::Unit> RederivedServiceWrapper::semifuture_onStopRequested() {
  auto [promise, future] = folly::makePromiseContract<folly::Unit>();
  call_cy_RederivedService_onStopRequested(
      this->if_object,
      std::move(promise)
  );
  return std::move(future);
}
} // namespace py3
} // namespace simple
