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

#include <utility>
#include <vector>

namespace thrift { namespace compiler { namespace py { namespace conv {

template <class T>
struct indexPtrVec {
  indexPtrVec(char const* name) {
    class_<vector<T*>> (name)
      .def(vector_indexing_suite<vector<T*>>());
    // This is a HACK as it tricks b::p into thinking that T* is a smart
    // pointer. At this point we don't care though because the objects we're
    // exposing are noncopyable/no_init and are going to live until the end of
    // the program
    register_ptr_to_python<T*>();
  }
};

template <class T>
struct indexVec {
  indexVec(char const* name) {
    class_<vector<T>> (name)
      .def(vector_indexing_suite<vector<T>>());
  }
};


template <class T, class U>
struct indexMap {
  typedef map<T, U> Map;

  struct iteration_helper {
    static list keys(Map const& self) {
      list t;
      for (const auto& v : self)
        t.append(v.first);
      return t;
    }
  };

  indexMap(char const* name) {
    class_<Map> (name)
      .def(map_indexing_suite<Map>())
      .def("keys", &iteration_helper::keys)
      ;
  }
};

template <class T, class U>
const T& TO(const U& from) {
  return static_cast<const T&>(from);
}

// Assumes Key and Val are pointers.
template<class Key, class Val>
struct map_item {
  typedef std::vector<std::pair<Key,Val>> Map;

  static list items(Map const& self) {
    list t;
    for (const auto& v : self)
      t.append( boost::python::make_tuple(boost::ref(v.first),
                                          boost::ref(v.second)));
    return t;
  }
};

}}}} // thrift::compiler::py::conv
