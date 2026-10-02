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

#include <thrift/lib/cpp2/protocol/detail/JsonReader.h>

#include <charconv>
#include <cstdint>
#include <limits>

#include <fmt/core.h>
#include <folly/Conv.h>
#include <folly/String.h>
#include <folly/Unicode.h>
#include <thrift/lib/cpp/protocol/TProtocolException.h>

namespace apache::thrift::json5::detail {

namespace {

constexpr bool isAsciiDigit(char c) {
  return c >= '0' && c <= '9';
}

constexpr bool isAsciiHexDigit(char c) {
  return isAsciiDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

constexpr bool isAsciiAlpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

constexpr bool isAsciiSpace(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' ||
      c == '\r';
}

bool isIdentifierStart(char c) {
  return isAsciiAlpha(c) || c == '_' || c == '$';
}

bool isIdentifierPart(char c) {
  return isIdentifierStart(c) || isAsciiDigit(c);
}

// Messages may echo input bytes, but must stay valid UTF-8 (thrift-python
// decodes them strictly).
[[noreturn]] void throwParseError(const std::string& msg) {
  throw protocol::TProtocolException(
      protocol::TProtocolException::INVALID_DATA,
      "Json5Reader: " + folly::backslashify(msg));
}

template <typename T>
T parseNumberOrThrow(std::string_view str) {
  auto result = folly::tryTo<T>(str);
  if (!result.hasValue()) {
    throwParseError(folly::makeConversionError(result.error(), str).what());
  }
  return *result;
}

std::uint64_t parseHexMagnitude(std::string_view str) {
  uint64_t uval = 0;
  auto [ptr, ec] =
      std::from_chars(str.data(), str.data() + str.size(), uval, 16);

  if (ec != std::errc{} || ptr != str.data() + str.size()) {
    throwParseError(fmt::format("invalid hex number {}", str));
  }
  return uval;
}

// Rejects integers outside the int64_t range.
Json5Reader::Integer makeInteger(
    bool negative, std::uint64_t magnitude, std::string_view literal) {
  constexpr std::uint64_t kInt64Max = std::numeric_limits<std::int64_t>::max();
  const std::uint64_t maxMagnitude = negative ? kInt64Max + 1 : kInt64Max;
  if (magnitude > maxMagnitude) {
    throwParseError(fmt::format("integer {} out of range", literal));
  }
  return {.negative = negative, .magnitude = magnitude};
}

std::uint16_t readFourHexDigits(folly::io::Cursor& cursor) {
  if (!cursor.canAdvance(4)) {
    throwParseError("expected 4 hex digits");
  }
  return static_cast<std::uint16_t>(
      parseHexMagnitude(cursor.readFixedString(4)));
}

// The writer escapes strings with folly::json::escapeString's validate_utf8, so
// accepting invalid UTF-8 would produce values that cannot be written back.
void validateUtf8(std::string_view str) {
  auto* p = reinterpret_cast<const unsigned char*>(str.data());
  auto* const end = p + str.size();
  while (p != end) {
    try {
      folly::utf8ToCodePoint(p, end, /*skipOnError=*/false);
    } catch (const std::exception& e) {
      throwParseError(fmt::format("invalid UTF-8 in string: {}", e.what()));
    }
  }
}

// Decodes a `\uXXXX` escape, with the cursor just past the `u`. A code point
// outside the BMP is written as a UTF-16 surrogate pair, i.e. two escapes.
void decodeUnicodeEscape(folly::io::Cursor& cursor, std::string& out) {
  const std::uint16_t first = readFourHexDigits(cursor);
  char32_t codePoint = first;

  if (folly::utf16_code_unit_is_high_surrogate(first)) {
    if (!cursor.canAdvance(2) || cursor.readFixedString(2) != "\\u") {
      throwParseError("expected a second escape to complete surrogate pair");
    }
    const std::uint16_t second = readFourHexDigits(cursor);
    if (!folly::utf16_code_unit_is_low_surrogate(second)) {
      throwParseError("invalid second half of surrogate pair");
    }
    codePoint =
        folly::unicode_code_point_from_utf16_surrogate_pair(first, second);
  } else if (!folly::utf16_code_unit_is_bmp(first)) {
    throwParseError("unpaired low surrogate");
  }

  folly::appendCodePointToUtf8(codePoint, out);
}

char peekNext(folly::io::Cursor& cursor) {
  return cursor.isAtEnd() ? '\0' : static_cast<char>(cursor.peek().front());
}

char readNext(folly::io::Cursor& cursor) {
  if (cursor.isAtEnd()) {
    throwParseError("unexpected end of input");
  }
  return cursor.read<char>();
}

} // namespace

// -- cursor operations ----------------------------------------------------

void Json5Reader::setCursor(folly::io::Cursor c) {
  in_ = std::move(c);
  skipWhitespaceAndComments();
}

const folly::io::Cursor& Json5Reader::getCursor() const {
  return in_.value();
}

folly::io::Cursor& Json5Reader::cursor() {
  return in_.value();
}

char Json5Reader::peekChar() {
  return peekNext(cursor());
}

char Json5Reader::readChar() {
  return readNext(cursor());
}

void Json5Reader::consume(char expected) {
  char got = readChar();
  if (got != expected) {
    throwParseError(fmt::format("expected '{}', got '{}'", expected, got));
  }
  skipWhitespaceAndComments();
}

// -- whitespace and comments ----------------------------------------------

bool Json5Reader::skipComment() {
  if (peekChar() != '/') {
    return false;
  }
  cursor().skip(1);
  switch (readChar()) {
    case '/':
      // Skip "// ..." comment
      cursor().skipWhile([](char c) { return c != '\n' && c != '\r'; });
      if (!cursor().isAtEnd()) {
        cursor().skip(1); // Skip "\n"
      }
      return true;
    case '*': {
      // Skip "/* ... */" comment
      char prev = 0;
      cursor().skipWhile([&prev](char curr) {
        if (prev == '*' && curr == '/') {
          return false;
        }
        prev = curr;
        return true;
      });
      if (cursor().isAtEnd()) {
        throwParseError("unterminated block comment");
      } else {
        cursor().skip(1); // skip "/"
      }
      return true;
    }
    default:
      throwParseError("expected '//' or '/*' comment");
  }
}

void Json5Reader::skipWhitespaceAndComments() {
  do {
    cursor().skipWhile(isAsciiSpace);
    // Loop to handle adjacent comments, e.g. `/*a*//*b*/`.
  } while (skipComment());
}

// -- comma handling -------------------------------------------------------

void Json5Reader::expectCommaOrEnd() {
  skipWhitespaceAndComments();
  char c = peekChar();
  if (c == ',') {
    cursor().skip(1);
    skipWhitespaceAndComments();
    if (peekChar() == ',') {
      throwParseError("unexpected consecutive commas"); // e.g., "[1,,2]"
    }
  } else if (c != ']' && c != '}' && c != '\0') {
    throwParseError(
        fmt::format("expected ',' or closing delimiter, got '{}'", c));
  }
}

// -- token peeking --------------------------------------------------------

Json5Reader::Token Json5Reader::peekToken() {
  switch (peekChar()) {
    case '[':
      return Token::ListBegin;
    case ']':
      return Token::ListEnd;
    case '{':
      return Token::ObjectBegin;
    case '}':
      return Token::ObjectEnd;
    case '\0':
      throwParseError("unexpected end of input");
    default:
      return Token::Primitive;
  }
}

// -- strings and identifiers ----------------------------------------------

std::string Json5Reader::parseString(char quote) {
  std::string result;
  bool hasRawNonAscii = false;
  while (true) {
    char c = readChar();
    if (c == quote) {
      // Escapes always decode to well-formed UTF-8; only raw bytes need
      // validation.
      if (hasRawNonAscii) {
        validateUtf8(result);
      }
      return result;
    }
    if (c == '\n' || c == '\r') {
      throwParseError("unescaped newline in string");
    }
    if (c != '\\') {
      hasRawNonAscii |= (static_cast<unsigned char>(c) & 0x80) != 0;
      result.push_back(c);
      continue;
    }
    char esc = readChar();
    switch (esc) {
      case '"':
      case '/':
      case '\'':
      case '\\':
      case '\n':
      case '\r':
        if (esc != '\n' && esc != '\r') {
          result.push_back(esc);
        }
        if (esc == '\r' && peekChar() == '\n') {
          // handles "\r\n" (CRLF)
          cursor().skip(1);
        }
        break;
      case 'b':
        result.push_back('\b');
        break;
      case 'f':
        result.push_back('\f');
        break;
      case 'n':
        result.push_back('\n');
        break;
      case 'r':
        result.push_back('\r');
        break;
      case 't':
        result.push_back('\t');
        break;
      case 'u':
        decodeUnicodeEscape(cursor(), result);
        break;
      default:
        throwParseError(fmt::format("unknown escape '\\{}' in string", esc));
    }
  }
}

std::string Json5Reader::readObjectName() {
  std::string name;
  char c = peekChar();
  if (c == '"' || c == '\'') {
    cursor().skip(1);
    name = parseString(c);
  } else if (isIdentifierStart(c)) {
    name = cursor().readWhile(isIdentifierPart);
  } else {
    throwParseError(fmt::format("expected object name, got '{}'", c));
  }
  skipWhitespaceAndComments();
  consume(':');
  return name;
}

// -- numbers --------------------------------------------------------------

Json5Reader::Primitive Json5Reader::parseNumber(
    folly::io::Cursor& cursor, FloatingPointPrecision precision) {
  std::string numStr;
  char c = peekNext(cursor);

  const int8_t sign = (c == '-' ? -1 : 1);
  if (c == '+' || c == '-') {
    numStr.push_back(readNext(cursor));
    c = peekNext(cursor);
  }

  // Infinity or NaN
  if (c == 'I' || c == 'N') {
    std::string word = cursor.readWhile(isIdentifierPart);
    if (word == "Infinity") {
      double d = std::copysign(std::numeric_limits<double>::infinity(), sign);
      if (precision == FloatingPointPrecision::Single) {
        return float(d);
      }
      return d;
    }
    if (word == "NaN") {
      double d = std::copysign(std::numeric_limits<double>::quiet_NaN(), sign);
      if (precision == FloatingPointPrecision::Single) {
        return float(d);
      }
      return d;
    }
    throwParseError(
        fmt::format("expected 'Infinity' or 'NaN', got '{}'", word));
  }
  // Hex literal: 0x...
  if (c == '0') {
    numStr.push_back(readNext(cursor));
    c = peekNext(cursor);
    if (c == 'x' || c == 'X') {
      cursor.skip(1);
      auto digits = cursor.readWhile(isAsciiHexDigit);
      return makeInteger(
          sign < 0, parseHexMagnitude(digits), numStr + "x" + digits);
    }
  } else if (c != '.') {
    numStr += cursor.readWhile(isAsciiDigit);
    c = peekNext(cursor);
  }

  bool isFloating = false;

  if (c == '.') {
    isFloating = true;
    numStr.push_back(readNext(cursor));
    numStr += cursor.readWhile(isAsciiDigit);
    c = peekNext(cursor);
  }

  if (c == 'e' || c == 'E') {
    isFloating = true;
    numStr.push_back(readNext(cursor));
    if (peekNext(cursor) == '+' || peekNext(cursor) == '-') {
      numStr.push_back(readNext(cursor));
    }
    numStr += cursor.readWhile(isAsciiDigit);
  }

  if (numStr.empty() || numStr == "+" || numStr == "-") {
    throwParseError("expected number");
  }

  if (!isFloating) {
    std::string_view digits = numStr;
    if (digits.front() == '+' || digits.front() == '-') {
      digits.remove_prefix(1);
    }
    return makeInteger(
        sign < 0, parseNumberOrThrow<std::uint64_t>(digits), numStr);
  }
  if (precision == FloatingPointPrecision::Single) {
    return parseNumberOrThrow<float>(numStr);
  }
  return parseNumberOrThrow<double>(numStr);
}

// -- values ---------------------------------------------------------------

Json5Reader::Primitive Json5Reader::readPrimitive(
    FloatingPointPrecision precision) {
  char c = peekChar();
  Primitive result;

  if (c == '"' || c == '\'') {
    cursor().skip(1);
    result = parseString(c);
  } else if (
      isAsciiDigit(c) || c == '-' || c == '+' || c == '.' || c == 'N' ||
      c == 'I') {
    result = parseNumber(cursor(), precision);
  } else if (isIdentifierStart(c)) {
    std::string word = cursor().readWhile(isIdentifierPart);
    if (word == "null") {
      result = std::monostate{};
    } else if (word == "true") {
      result = true;
    } else if (word == "false") {
      result = false;
    } else {
      throwParseError(fmt::format("unexpected identifier '{}'", word));
    }
  } else {
    throwParseError(fmt::format("expected value, got '{}'", c));
  }

  skipWhitespaceAndComments();
  expectCommaOrEnd();
  return result;
}

} // namespace apache::thrift::json5::detail
