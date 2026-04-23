cmake_minimum_required(VERSION 3.16)
project(fbthrift_protocol_fuzzer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(THRIFT_FUZZ_SANITIZER_FLAGS "-fsanitize=fuzzer,address,undefined" CACHE STRING
    "Sanitizers used when building fuzz_thrift")
separate_arguments(THRIFT_FUZZ_SANITIZER_FLAG_LIST NATIVE_COMMAND "${THRIFT_FUZZ_SANITIZER_FLAGS}")

get_filename_component(THRIFT_SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)

set(GEN_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
file(MAKE_DIRECTORY
  "${GEN_INCLUDE_DIR}/thrift/lib/cpp" "${GEN_INCLUDE_DIR}/thrift/lib/cpp/util"
  "${GEN_INCLUDE_DIR}/folly/lang" "${GEN_INCLUDE_DIR}/folly/portability"
  "${GEN_INCLUDE_DIR}/folly/io/async")
file(WRITE "${GEN_INCLUDE_DIR}/thrift/lib/cpp/thrift_config.h" "#pragma once\n")
file(WRITE "${GEN_INCLUDE_DIR}/thrift/lib/cpp/util/VarintUtils.h" [=[
#pragma once
#include <cstdint>
namespace apache::thrift::util {
constexpr uint32_t i32ToZigzag(int32_t n){return (static_cast<uint32_t>(n)<<1)^static_cast<uint32_t>(n>>31);} constexpr uint64_t i64ToZigzag(int64_t n){return (static_cast<uint64_t>(n)<<1)^static_cast<uint64_t>(n>>63);} inline int32_t zigzagToI32(uint32_t n){return static_cast<int32_t>((n>>1)^(~(n&1)+1));} inline int64_t zigzagToI64(uint64_t n){return static_cast<int64_t>((n>>1)^(~(n&1)+1));}
}
]=])
file(WRITE "${GEN_INCLUDE_DIR}/folly/_compat.h" [=[
#pragma once
#include <bit>
#include <cstdint>
#include <exception>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>
#define FOLLY_EXPORT
#define FOLLY_PUSH_WARNING
#define FOLLY_POP_WARNING
#define FOLLY_CLANG_DISABLE_WARNING(x)
#define FOLLY_PACK_ATTR __attribute__((__packed__))
#define FOLLY_ALWAYS_INLINE inline __attribute__((__always_inline__))
#define FOLLY_NOINLINE __attribute__((__noinline__))
#if defined(__GNUC__) || defined(__clang__)
#define FOLLY_LIKELY(x) __builtin_expect(!!(x), 1)
#define FOLLY_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define FOLLY_LIKELY(x) (x)
#define FOLLY_UNLIKELY(x) (x)
#endif
#define LIKELY(x) FOLLY_LIKELY(x)
#define UNLIKELY(x) FOLLY_UNLIKELY(x)
namespace folly {
using StringPiece = std::string_view; template <typename Iter> class Range{public: constexpr Range():b_(),e_(){} constexpr Range(Iter b, Iter e):b_(b),e_(e){} constexpr Iter begin()const{return b_;} constexpr Iter end()const{return e_;} private: Iter b_,e_;}; class fbstring:public std::string{public: using std::string::string; fbstring(const std::string& v):std::string(v){} fbstring(std::string&& v) noexcept:std::string(std::move(v)){} fbstring& operator=(const std::string& v){std::string::operator=(v);return *this;} fbstring& operator=(std::string&& v) noexcept{std::string::operator=(std::move(v));return *this;} std::string toStdString() const{return *this;}}; template <typename T, typename A = std::allocator<T>> using fbvector = std::vector<T, A>; template <typename Out, typename... Args> Out to(Args&&... args){std::ostringstream os; (os << ... << args); return Out(os.str());} template <typename Enum, typename = std::enable_if_t<std::is_enum<Enum>::value>> constexpr auto to_underlying(Enum v) noexcept{return static_cast<std::underlying_type_t<Enum>>(v);} template <typename...> inline constexpr bool always_false = false; using std::bit_cast; template <typename T> constexpr T swapBytes(T v){using U = std::make_unsigned_t<T>; if constexpr (sizeof(T)==1) return v; if constexpr (sizeof(T)==2) return static_cast<T>(__builtin_bswap16(static_cast<U>(v))); if constexpr (sizeof(T)==4) return static_cast<T>(__builtin_bswap32(static_cast<U>(v))); return static_cast<T>(__builtin_bswap64(static_cast<U>(v)));} struct Endian{template <typename T> static constexpr T big(T v){if constexpr (std::endian::native==std::endian::big) return v; return swapBytes(v);} template <typename T> static constexpr T little(T v){if constexpr (std::endian::native==std::endian::little) return v; return swapBytes(v);}}; class AsyncSocketException: public std::exception{public: enum AsyncSocketExceptionType{UNKNOWN=0}; AsyncSocketException(AsyncSocketExceptionType t=UNKNOWN,std::string m="",int e=0):type_(t),msg_(std::move(m)),err_(e){} AsyncSocketExceptionType getType() const noexcept{return type_;} int getErrno() const noexcept{return err_;} const char* what() const noexcept override{return msg_.c_str();} private: AsyncSocketExceptionType type_; std::string msg_; int err_;};
}
]=])
foreach(hdr Conv.h FBString.h FBVector.h Likely.h Range.h Traits.h lang/Bits.h portability/Time.h portability/SysTime.h io/async/AsyncSocketException.h)
  file(WRITE "${GEN_INCLUDE_DIR}/folly/${hdr}" "#pragma once\n#include <folly/_compat.h>\n")
endforeach()

add_executable(fuzz_thrift fuzz_thrift.cpp)
target_include_directories(fuzz_thrift PRIVATE "${GEN_INCLUDE_DIR}" "${THRIFT_SOURCE_ROOT}")
target_compile_options(fuzz_thrift PRIVATE ${THRIFT_FUZZ_SANITIZER_FLAG_LIST})
target_link_options(fuzz_thrift PRIVATE ${THRIFT_FUZZ_SANITIZER_FLAG_LIST})
