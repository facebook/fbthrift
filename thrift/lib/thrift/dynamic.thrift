// Copyright (c) 2006- Facebook
// Distributed under the Thrift Software License
//
// See accompanying file LICENSE or visit the Thrift site at:
// http://developers.facebook.com/thrift/

namespace cpp apache.thrift
namespace php apache.thrift
namespace py apache.thrift.dynamic

cpp_include "thrift/lib/thrift/SerializableDynamic.h"

union Dynamic {
  1: bool boolean;
  2: i64 integer;
  3: double doubl;
  4: string str;
  5: list<Dynamic> array;
  6: map<string, Dynamic> object;
} (cpp.type = "::apache::thrift::SerializableDynamic")
