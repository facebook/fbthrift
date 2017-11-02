/*
 * Copyright 2017-present Facebook, Inc.
 *
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

#pragma once

/* avoid problems when included in Cython files */
#ifdef T_BOOL
#pragma push_macro("T_BOOL")
#pragma push_macro("T_BYTE")
#pragma push_macro("T_DOUBLE")
#pragma push_macro("T_FLOAT")
#pragma push_macro("T_STRING")
#undef T_BOOL
#undef T_BYTE
#undef T_DOUBLE
#undef T_FLOAT
#undef T_STRING
#define CPP2_SERIALIZER_PYTHON_SAVE 1
#endif

#include <thrift/lib/cpp2/protocol/Serializer.h>

#ifdef CPP2_SERIALIZER_PYTHON_SAVE
#pragma pop_macro("T_BOOL")
#pragma pop_macro("T_BYTE")
#pragma pop_macro("T_DOUBLE")
#pragma pop_macro("T_FLOAT")
#pragma pop_macro("T_STRING")
#undef CPP2_SERIALIZED_PYTHON_SAVE
#endif
