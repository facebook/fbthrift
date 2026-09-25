/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <thrift/lib/cpp2/protocol/JSONProtocolCommon.h>

#include <random>
#include <string>

#include <gtest/gtest.h>
#include <folly/String.h>
#include <folly/io/IOBuf.h>

using namespace std::literals;

class JSONProtocolCommonTest : public testing::Test {};

namespace {

auto quote(std::string_view sv) {
  return "\"" + folly::cEscape<std::string>(sv) + "\"";
}

template <typename Fn>
void each_split(std::string_view sv, Fn fn) {
  for (size_t i = 0; i <= sv.size(); ++i) {
    fn(i, std::array{sv.substr(0, i), sv.substr(i)});
  }
}

template <typename Vec>
std::unique_ptr<folly::IOBuf> from_split(Vec const& vec) {
  //  cursor transparently handles empty bufs
  auto buf = folly::IOBuf::create(0);
  for (auto sv : vec) {
    buf->appendToChain(folly::IOBuf::copyBuffer(folly::StringPiece(sv)));
  }
  return buf;
}

} // namespace

TEST_F(JSONProtocolCommonTest, whitespace_combinatorics) {
  //  test behavior of member readWhitespace with a large suite of inputs

  struct Reader : apache::thrift::JSONProtocolReaderCommon {
    using apache::thrift::JSONProtocolReaderCommon::readWhitespace;
  };

  constexpr size_t big = 40;

  std::mt19937 rng;

  auto const spaces = [](size_t const len) { //
    return std::string(len, ' ');
  };
  auto const whitespaces = [&](size_t const len) {
    std::string s;
    for (size_t i = 0; i < len; ++i) {
      s += " \r\n\t"[rng() % 4];
    }
    return s;
  };
  auto const blackspace = "[[[blackspace]]]";

  auto read = [&](auto const& svs) {
    auto const buf = from_split(svs);
    Reader r;
    r.setInput(buf.get());
    auto const pos0 = r.getCursorPosition();
    auto const len = r.readWhitespace();
    auto const pos1 = r.getCursorPosition();
    EXPECT_EQ(len, pos1 - pos0);
    return len;
  };

  //  spaces
  for (size_t i = 0; i <= big; ++i) {
    auto const s = spaces(i);
    each_split(s, [&](auto const& split, auto const& vec) {
      EXPECT_EQ(i, read(vec)) //
          << "split[" << split << "] input: " << quote(s);
    });
  }

  //  spaces then text
  for (size_t i = 0; i <= big; ++i) {
    auto const s = spaces(i) + blackspace;
    each_split(s, [&](auto const& split, auto const& vec) {
      EXPECT_EQ(i, read(vec)) //
          << "split[" << split << "] input: " << quote(s);
    });
  }

  //  newline then spaces
  for (size_t i = 0; i <= big; ++i) {
    auto const s = "\n" + spaces(i);
    each_split(s, [&](auto const& split, auto const& vec) {
      EXPECT_EQ(i + 1, read(vec))
          << "split[" << split << "] input: " << quote(s);
    });
  }

  //  newline then spaces then text
  for (size_t i = 0; i <= big; ++i) {
    auto const s = "\n" + spaces(i) + blackspace;
    each_split(s, [&](auto const& split, auto const& vec) {
      EXPECT_EQ(i + 1, read(vec))
          << "split[" << split << "] input: " << quote(s);
    });
  }

  //  whitespaces
  for (size_t i = 0; i <= big; ++i) {
    auto const s = whitespaces(i);
    each_split(s, [&](auto const& split, auto const& vec) {
      EXPECT_EQ(i, read(vec)) //
          << "split[" << split << "] input: " << quote(s);
    });
  }

  //  whitespaces then text
  for (size_t i = 0; i <= big; ++i) {
    auto const s = whitespaces(i) + blackspace;
    each_split(s, [&](auto const& split, auto const& vec) {
      EXPECT_EQ(i, read(vec)) //
          << "split[" << split << "] input: " << quote(s);
    });
  }

  //  newline then whitespaces
  for (size_t i = 0; i <= big; ++i) {
    auto const s = "\n" + whitespaces(i);
    each_split(s, [&](auto const& split, auto const& vec) {
      EXPECT_EQ(i + 1, read(vec))
          << "split[" << split << "] input: " << quote(s);
    });
  }

  //  newline then whitespaces then text
  for (size_t i = 0; i <= big; ++i) {
    auto const s = "\n" + whitespaces(i) + blackspace;
    each_split(s, [&](auto const& split, auto const& vec) {
      EXPECT_EQ(i + 1, read(vec))
          << "split[" << split << "] input: " << quote(s);
    });
  }
}

TEST_F(JSONProtocolCommonTest, string_ascii) {
  struct Reader : apache::thrift::JSONProtocolReaderCommon {
    using apache::thrift::JSONProtocolReaderCommon::readJSONString;
  };
  struct Writer : apache::thrift::JSONProtocolWriterCommon {
    using apache::thrift::JSONProtocolWriterCommon::writeJSONString;
  };

  auto write = [](std::string_view sv) {
    folly::IOBufQueue q;
    Writer w;
    w.setOutput(&q);
    w.writeJSONString(sv);
    return std::string(folly::StringPiece(q.move()->coalesce()));
  };

  auto read = [](auto const& svs) {
    auto const buf = from_split(svs);
    Reader r;
    r.setInput(buf.get());
    std::string out;
    r.readJSONString(out);
    // NOLINTNEXTLINE(clang-diagnostic-nrvo) false-positive?
    return out;
  };

  for (size_t i = 0; i < 40; ++i) {
    auto const s = std::invoke([&] {
      std::string r;
      r += std::string(i, 'x');
      return r;
    });
    EXPECT_EQ(i, s.size());
    auto v = write(s);
    EXPECT_EQ(i + 2, v.size());
    each_split(v, [&](auto const& split, auto const& vec) {
      EXPECT_EQ(s, read(vec)) //
          << "split[" << split << "] input: " << quote(s);
    });
  }

  for (size_t i = 0; i < 40; ++i) {
    auto const s = std::invoke([&] {
      std::string r;
      for (size_t j = 0; j < i; ++j) {
        r += std::string("x\"y\"z");
      }
      return r;
    });
    EXPECT_EQ(5 * i, s.size());
    auto v = write(s);
    EXPECT_EQ(7 * i + 2, v.size());
    each_split(v, [&](auto const& split, auto const& vec) {
      EXPECT_EQ(s, read(vec)) //
          << "split[" << split << "] input: " << quote(s);
    });
  }
}

TEST_F(JSONProtocolCommonTest, string_ascii_underflow) {
  struct Reader : apache::thrift::JSONProtocolReaderCommon {
    using apache::thrift::JSONProtocolReaderCommon::readJSONString;
  };

  auto read = [](auto const& svs) {
    auto const buf = from_split(svs);
    Reader r;
    r.setInput(buf.get());
    std::string out;
    r.readJSONString(out);
    // NOLINTNEXTLINE(clang-diagnostic-nrvo) false-positive?
    return out;
  };

  {
    auto v = "\"hello"s; // no terminal "
    each_split(v, [&](auto, auto const& vec) {
      EXPECT_THROW(read(vec), std::out_of_range);
    });
  }
}

TEST_F(JSONProtocolCommonTest, string_ascii_stress) {
  struct Reader : apache::thrift::JSONProtocolReaderCommon {
    using apache::thrift::JSONProtocolReaderCommon::readJSONString;
  };
  struct Writer : apache::thrift::JSONProtocolWriterCommon {
    using apache::thrift::JSONProtocolWriterCommon::writeJSONString;
  };

  std::mt19937 rng;
  for (size_t i = 0; i < 1024; ++i) {
    std::uniform_int_distribution<size_t> length_d{0, 16};
    std::uniform_int_distribution<uint16_t> char_d{0x00, 0x7f};
    std::string e(length_d(rng), 0);
    std::generate(e.begin(), e.end(), [&] { return char(char_d(rng)); });

    folly::IOBufQueue q;
    Writer w;
    w.setOutput(&q);
    w.writeJSONString(e);

    std::string f;
    Reader r;
    r.setInput(q.front());
    r.readJSONString(f);

    EXPECT_EQ(e, f);
  }
}

TEST_F(JSONProtocolCommonTest, peekValueTType) {
  using TType = apache::thrift::protocol::TType;

  auto peek = [](std::string_view json) {
    auto buf = folly::IOBuf::copyBuffer(json);
    apache::thrift::JSONProtocolReaderCommon reader;
    reader.setInput(buf.get());
    return reader.peekValueTType();
  };

  // JSON object → T_STRUCT
  EXPECT_EQ(TType::T_STRUCT, peek("{"));
  EXPECT_EQ(TType::T_STRUCT, peek("  {"));

  // JSON array → T_LIST
  EXPECT_EQ(TType::T_LIST, peek("["));
  EXPECT_EQ(TType::T_LIST, peek("  ["));

  // JSON string → T_STRING
  EXPECT_EQ(TType::T_STRING, peek("\"hello\""));
  EXPECT_EQ(TType::T_STRING, peek("  \"\""));

  // JSON number → T_DOUBLE
  EXPECT_EQ(TType::T_DOUBLE, peek("42"));
  EXPECT_EQ(TType::T_DOUBLE, peek("-1"));
  EXPECT_EQ(TType::T_DOUBLE, peek("+3"));
  EXPECT_EQ(TType::T_DOUBLE, peek("0.5"));
  EXPECT_EQ(TType::T_DOUBLE, peek("  123"));

  // JSON boolean → T_BOOL
  EXPECT_EQ(TType::T_BOOL, peek("true"));
  EXPECT_EQ(TType::T_BOOL, peek("false"));
  EXPECT_EQ(TType::T_BOOL, peek("  true"));

  // JSON null → T_VOID
  EXPECT_EQ(TType::T_VOID, peek("null"));
  EXPECT_EQ(TType::T_VOID, peek("  null"));

  // Empty / EOF → T_VOID
  EXPECT_EQ(TType::T_VOID, peek(""));
  EXPECT_EQ(TType::T_VOID, peek("  "));
}

TEST_F(JSONProtocolCommonTest, peekValueTType_does_not_consume) {
  using TType = apache::thrift::protocol::TType;

  // Verify peeking doesn't advance the cursor
  auto buf = folly::IOBuf::copyBuffer("\"hello\"");
  apache::thrift::JSONProtocolReaderCommon reader;
  reader.setInput(buf.get());

  auto pos_before = reader.getCursorPosition();
  auto ttype = reader.peekValueTType();
  auto pos_after = reader.getCursorPosition();

  EXPECT_EQ(TType::T_STRING, ttype);
  EXPECT_EQ(pos_before, pos_after);
}

TEST_F(JSONProtocolCommonTest, isJsonTypeCompatible) {
  using TType = apache::thrift::protocol::TType;
  using apache::thrift::isJsonTypeCompatible;

  // Exact match always compatible
  EXPECT_TRUE(isJsonTypeCompatible(TType::T_STRING, TType::T_STRING));
  EXPECT_TRUE(isJsonTypeCompatible(TType::T_BOOL, TType::T_BOOL));
  EXPECT_TRUE(isJsonTypeCompatible(TType::T_DOUBLE, TType::T_DOUBLE));
  EXPECT_TRUE(isJsonTypeCompatible(TType::T_LIST, TType::T_LIST));
  EXPECT_TRUE(isJsonTypeCompatible(TType::T_STRUCT, TType::T_STRUCT));

  // T_VOID (null/unknown) is compatible with anything
  EXPECT_TRUE(isJsonTypeCompatible(TType::T_VOID, TType::T_STRING));
  EXPECT_TRUE(isJsonTypeCompatible(TType::T_VOID, TType::T_I32));
  EXPECT_TRUE(isJsonTypeCompatible(TType::T_VOID, TType::T_BOOL));
  EXPECT_TRUE(isJsonTypeCompatible(TType::T_VOID, TType::T_LIST));

  // JSON number (T_DOUBLE) compatible with all numeric thrift types
  EXPECT_TRUE(isJsonTypeCompatible(TType::T_DOUBLE, TType::T_BYTE));
  EXPECT_TRUE(isJsonTypeCompatible(TType::T_DOUBLE, TType::T_I16));
  EXPECT_TRUE(isJsonTypeCompatible(TType::T_DOUBLE, TType::T_I32));
  EXPECT_TRUE(isJsonTypeCompatible(TType::T_DOUBLE, TType::T_I64));
  EXPECT_TRUE(isJsonTypeCompatible(TType::T_DOUBLE, TType::T_FLOAT));

  // JSON number NOT compatible with non-numeric types
  EXPECT_FALSE(isJsonTypeCompatible(TType::T_DOUBLE, TType::T_STRING));
  EXPECT_FALSE(isJsonTypeCompatible(TType::T_DOUBLE, TType::T_BOOL));
  EXPECT_FALSE(isJsonTypeCompatible(TType::T_DOUBLE, TType::T_LIST));
  EXPECT_FALSE(isJsonTypeCompatible(TType::T_DOUBLE, TType::T_STRUCT));

  // JSON object (T_STRUCT) compatible with T_MAP
  EXPECT_TRUE(isJsonTypeCompatible(TType::T_STRUCT, TType::T_MAP));
  EXPECT_FALSE(isJsonTypeCompatible(TType::T_STRUCT, TType::T_LIST));
  EXPECT_FALSE(isJsonTypeCompatible(TType::T_STRUCT, TType::T_STRING));

  // JSON array (T_LIST) compatible with T_SET
  EXPECT_TRUE(isJsonTypeCompatible(TType::T_LIST, TType::T_SET));
  EXPECT_FALSE(isJsonTypeCompatible(TType::T_LIST, TType::T_MAP));
  EXPECT_FALSE(isJsonTypeCompatible(TType::T_LIST, TType::T_STRING));

  // Incompatible types
  EXPECT_FALSE(isJsonTypeCompatible(TType::T_STRING, TType::T_I32));
  EXPECT_FALSE(isJsonTypeCompatible(TType::T_STRING, TType::T_BOOL));
  EXPECT_FALSE(isJsonTypeCompatible(TType::T_BOOL, TType::T_STRING));
  EXPECT_FALSE(isJsonTypeCompatible(TType::T_BOOL, TType::T_I32));

  // Map key: JSON map keys are always strings, so T_STRING is compatible
  // with numeric types when isMapKey=true
  EXPECT_TRUE(isJsonTypeCompatible(TType::T_STRING, TType::T_BYTE, true));
  EXPECT_TRUE(isJsonTypeCompatible(TType::T_STRING, TType::T_I16, true));
  EXPECT_TRUE(isJsonTypeCompatible(TType::T_STRING, TType::T_I32, true));
  EXPECT_TRUE(isJsonTypeCompatible(TType::T_STRING, TType::T_I64, true));
  EXPECT_TRUE(isJsonTypeCompatible(TType::T_STRING, TType::T_FLOAT, true));
  EXPECT_TRUE(isJsonTypeCompatible(TType::T_STRING, TType::T_DOUBLE, true));

  // Map key: T_STRING still not compatible with non-numeric types
  EXPECT_FALSE(isJsonTypeCompatible(TType::T_STRING, TType::T_BOOL, true));
  EXPECT_FALSE(isJsonTypeCompatible(TType::T_STRING, TType::T_LIST, true));
  EXPECT_FALSE(isJsonTypeCompatible(TType::T_STRING, TType::T_STRUCT, true));

  // Non-map-key: T_STRING still incompatible with numeric types
  EXPECT_FALSE(isJsonTypeCompatible(TType::T_STRING, TType::T_I32, false));
}

TEST_F(JSONProtocolCommonTest, readJSONIntegral_fast_path) {
  // Fast path: the whole integer token *and* its terminating non-number byte
  // live in a single contiguous peek buffer, so the token is parsed in place
  // (no std::string allocation) and the cursor is advanced past the digits
  // only, leaving the delimiter unconsumed.
  struct Reader : apache::thrift::JSONProtocolReaderCommon {
    using apache::thrift::JSONProtocolReaderCommon::readJSONIntegral;
  };

  auto read = [](std::string_view sv) {
    auto const buf = folly::IOBuf::copyBuffer(sv);
    Reader r;
    r.setInput(buf.get());
    int64_t val = 0;
    r.readJSONIntegral<int64_t>(val);
    return std::pair<int64_t, size_t>{val, r.getCursorPosition()};
  };

  using P = std::pair<int64_t, size_t>;
  EXPECT_EQ((P{123, 3}), read("123,"));
  EXPECT_EQ((P{-7, 2}), read("-7}"));
  EXPECT_EQ((P{0, 1}), read("0 "));
  EXPECT_EQ((P{-987654321, 10}), read("-987654321]"));
  // leading whitespace is consumed by readWhitespace before the fast path,
  // so the final position accounts for both the whitespace and the digits
  EXPECT_EQ((P{42, 5}), read("   42]"));
}

TEST_F(JSONProtocolCommonTest, readJSONIntegral_slow_path) {
  // Slow path: the token reaches the end of the current peek buffer
  // (size == peek.size()), so the reader falls back to readNumericalChars,
  // which stitches the number back together across buffer boundaries and also
  // handles the EOF-terminated case.
  struct Reader : apache::thrift::JSONProtocolReaderCommon {
    using apache::thrift::JSONProtocolReaderCommon::readJSONIntegral;
  };

  auto read = [](auto const& svs) {
    auto const buf = from_split(svs);
    Reader r;
    r.setInput(buf.get());
    int64_t val = 0;
    r.readJSONIntegral<int64_t>(val);
    return std::pair<int64_t, size_t>{val, r.getCursorPosition()};
  };

  using P = std::pair<int64_t, size_t>;
  // token split across two buffers, EOF-terminated
  EXPECT_EQ((P{12345, 5}), read(std::array{"123"sv, "45"sv}));
  // token split across two buffers, delimiter in the second buffer
  EXPECT_EQ((P{12345, 5}), read(std::array{"123"sv, "45,"sv}));
  // negative token split across two buffers
  EXPECT_EQ((P{-99, 3}), read(std::array{"-9"sv, "9]"sv}));
  // single buffer, EOF-terminated (the whole buffer is the token)
  EXPECT_EQ((P{678, 3}), read(std::array{"678"sv}));
}

TEST_F(JSONProtocolCommonTest, readJSONIntegral_split_combinatorics) {
  // For every buffer-boundary split of "<number><delimiter>", the parsed value
  // and final cursor position must match, whether the fast path (delimiter
  // lands in the peek buffer) or the slow path (token ends at a buffer
  // boundary) is taken.
  struct Reader : apache::thrift::JSONProtocolReaderCommon {
    using apache::thrift::JSONProtocolReaderCommon::readJSONIntegral;
  };

  auto read = [](auto const& svs) {
    auto const buf = from_split(svs);
    Reader r;
    r.setInput(buf.get());
    int64_t val = 0;
    r.readJSONIntegral<int64_t>(val);
    return std::pair<int64_t, size_t>{val, r.getCursorPosition()};
  };

  struct Case {
    std::string_view token;
    int64_t value;
  };
  constexpr Case cases[] = {
      {"0"sv, 0},
      {"7"sv, 7},
      {"-1"sv, -1},
      {"12345"sv, 12345},
      {"-987654321"sv, -987654321},
  };
  for (auto const& c : cases) {
    auto const s = std::string(c.token) + ","; // delimiter terminates the token
    each_split(s, [&](auto const& split, auto const& vec) {
      auto const [val, pos] = read(vec);
      EXPECT_EQ(c.value, val) //
          << "split[" << split << "] input: " << quote(s);
      EXPECT_EQ(c.token.size(), pos)
          << "split[" << split << "] input: " << quote(s);
    });
  }
}
