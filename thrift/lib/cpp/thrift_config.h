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
#ifndef THRIFT_THRIFT_CONFIG_H_
#define THRIFT_THRIFT_CONFIG_H_

#include <features.h>

/* Define to 1 if you have the `clock_gettime' function. */
#define THRIFT_HAVE_CLOCK_GETTIME 1

/* Define to 1 if you have the <endian.h> header file. */
#define THRIFT_HAVE_ENDIAN_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define THRIFT_HAVE_INTTYPES_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define THRIFT_HAVE_STDINT_H 1

/* Possible value for SIGNED_RIGHT_SHIFT_IS */
#define ARITHMETIC_RIGHT_SHIFT 1

/* Possible value for SIGNED_RIGHT_SHIFT_IS */
#define LOGICAL_RIGHT_SHIFT 2

/* Possible value for SIGNED_RIGHT_SHIFT_IS */
#define UNKNOWN_RIGHT_SHIFT 3

/* Indicates the effect of the right shift operator on negative signed
   integers */
#define SIGNED_RIGHT_SHIFT_IS 1

#endif /* THRIFT_THRIFT_CONFIG_H_ */
