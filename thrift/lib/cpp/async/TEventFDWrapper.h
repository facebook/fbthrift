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
/**
 * Work around the lack of <sys/eventfd.h> on glibc 2.5.1 which we still
 * need to support, sigh.
 *
 * Copyright 2013 Facebook
 * @author Tudor Bosman (tudorb@fb.com)
 */

#ifndef THRIFT_ASYNC_TEVENTFDWRAPPER_H_
#define THRIFT_ASYNC_TEVENTFDWRAPPER_H_ 1

#include <features.h>

// <sys/eventfd.h> doesn't exist on older glibc versions
#if (defined(__GLIBC__) && __GLIBC_PREREQ(2, 9))
#include <sys/eventfd.h>
#else /* !(defined(__GLIBC__) && __GLIBC_PREREQ(2, 9)) */

#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>

// Use existing __NR_eventfd2 if already defined
// Values from the Linux kernel source:
// arch/x86/include/asm/unistd_{32,64}.h
#ifndef __NR_eventfd2
#if defined(__x86_64__)
#define __NR_eventfd2  290
#elif defined(__i386__)
#define __NR_eventfd2  328
#else
#error "Can't define __NR_eventfd2 for your architecture."
#endif
#endif

enum
  {
    EFD_SEMAPHORE = 1,
#define EFD_SEMAPHORE EFD_SEMAPHORE
    EFD_CLOEXEC = 02000000,
#define EFD_CLOEXEC EFD_CLOEXEC
    EFD_NONBLOCK = 04000
#define EFD_NONBLOCK EFD_NONBLOCK
  };

// http://www.kernel.org/doc/man-pages/online/pages/man2/eventfd.2.html
// Use the eventfd2 system call, as in glibc 2.9+
// (requires kernel 2.6.30+)
#define eventfd(initval, flags) syscall(__NR_eventfd2,(initval),(flags))

#endif /* !(defined(__GLIBC__) && __GLIBC_PREREQ(2, 9)) */

#endif /* THRIFT_LIB_CPP_ASYNC_TEVENTFDWRAPPER_H_ */
