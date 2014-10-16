/*
 * Copyright 2014 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _LIB_CPP_THRIFT_CONFIG_H
#define _LIB_CPP_THRIFT_CONFIG_H 1

/* lib/cpp/thrift_config.h. Generated automatically at end of configure. */
/* config.h.  Generated from config.hin by configure.  */
/* config.hin.  Generated from configure.ac by autoheader.  */

/* Define if the AI_ADDRCONFIG symbol is unavailable */
/* #undef AI_ADDRCONFIG */

/* Possible value for SIGNED_RIGHT_SHIFT_IS */
#ifndef THRIFT_ARITHMETIC_RIGHT_SHIFT
#define THRIFT_ARITHMETIC_RIGHT_SHIFT 1
#endif

/* Define to one of `_getb67', `GETB67', `getb67' for Cray-2 and Cray-YMP
   systems. This function is required for `alloca.c' support on those systems.
   */
/* #undef CRAY_STACKSEG_END */

/* Define to 1 if using `alloca.c'. */
/* #undef C_ALLOCA */

/* Define to 1 if you have the `alarm' function. */
#ifndef THRIFT_HAVE_ALARM
#define THRIFT_HAVE_ALARM 1
#endif

/* Define to 1 if you have `alloca', as a function or macro. */
#ifndef THRIFT_HAVE_ALLOCA
#define THRIFT_HAVE_ALLOCA 1
#endif

/* Define to 1 if you have <alloca.h> and it should be used (not on Ultrix).
   */
#ifndef THRIFT_HAVE_ALLOCA_H
#define THRIFT_HAVE_ALLOCA_H 1
#endif

/* Define to 1 if you have the <arpa/inet.h> header file. */
#ifndef THRIFT_HAVE_ARPA_INET_H
#define THRIFT_HAVE_ARPA_INET_H 1
#endif

/* define if the Boost library is available */
#ifndef THRIFT_HAVE_BOOST
#define THRIFT_HAVE_BOOST /**/
#endif

/* define if the Boost::Python library is available */
#ifndef THRIFT_HAVE_BOOST_PYTHON
#define THRIFT_HAVE_BOOST_PYTHON /**/
#endif

/* define if the Boost::Thread library is available */
#ifndef THRIFT_HAVE_BOOST_THREAD
#define THRIFT_HAVE_BOOST_THREAD /**/
#endif

/* Define to 1 if you have the `bzero' function. */
#ifndef THRIFT_HAVE_BZERO
#define THRIFT_HAVE_BZERO 1
#endif

/* Define to 1 if you have the `clock_gettime' function. */
#if defined(__linux__) || defined(__FreeBSD__)
#ifndef THRIFT_HAVE_CLOCK_GETTIME
#define THRIFT_HAVE_CLOCK_GETTIME 1
#endif
#endif // defined(__linux__) || defined(__FreeBSD__)

/* Define to 1 if you have the declaration of `strerror_r', and to 0 if you
   don't. */
#ifndef THRIFT_HAVE_DECL_STRERROR_R
#define THRIFT_HAVE_DECL_STRERROR_R 1
#endif

/* Define to 1 if you have the <dlfcn.h> header file. */
#ifndef THRIFT_HAVE_DLFCN_H
#define THRIFT_HAVE_DLFCN_H 1
#endif

/* Define to 1 if you don't have `vprintf' but do have `_doprnt.' */
/* #undef HAVE_DOPRNT */

/* Define to 1 if you have the <fcntl.h> header file. */
#ifndef THRIFT_HAVE_FCNTL_H
#define THRIFT_HAVE_FCNTL_H 1
#endif

/* Define to 1 if you have the `fork' function. */
#ifndef THRIFT_HAVE_FORK
#define THRIFT_HAVE_FORK 1
#endif

/* Define to 1 if you have the `ftruncate' function. */
#ifndef THRIFT_HAVE_FTRUNCATE
#define THRIFT_HAVE_FTRUNCATE 1
#endif

/* Define to 1 if you have the `gethostbyname' function. */
#ifndef THRIFT_HAVE_GETHOSTBYNAME
#define THRIFT_HAVE_GETHOSTBYNAME 1
#endif

/* Define to 1 if you have the `gettimeofday' function. */
#ifndef THRIFT_HAVE_GETTIMEOFDAY
#define THRIFT_HAVE_GETTIMEOFDAY 1
#endif

/* Define to 1 if you have the <inttypes.h> header file. */
#ifndef THRIFT_HAVE_INTTYPES_H
#define THRIFT_HAVE_INTTYPES_H 1
#endif

/* define if libevent is available */
#ifndef THRIFT_HAVE_LIBEVENT
#define THRIFT_HAVE_LIBEVENT /**/
#endif

/* Define to 1 if you have the `folly' library (-lfolly). */
#ifndef THRIFT_HAVE_LIBFOLLY
#define THRIFT_HAVE_LIBFOLLY 1
#endif

/* Define to 1 if you have the `gflags' library (-lgflags). */
#ifndef THRIFT_HAVE_LIBGFLAGS
#define THRIFT_HAVE_LIBGFLAGS 1
#endif

/* Define to 1 if you have the `glog' library (-lglog). */
#ifndef THRIFT_HAVE_LIBGLOG
#define THRIFT_HAVE_LIBGLOG 1
#endif

/* Define to 1 if you have the <libintl.h> header file. */
#ifndef THRIFT_HAVE_LIBINTL_H
#define THRIFT_HAVE_LIBINTL_H 1
#endif

/* Define to 1 if you have the `numa' library (-lnuma). */
#ifndef THRIFT_HAVE_LIBNUMA
#define THRIFT_HAVE_LIBNUMA 1
#endif

/* Define to 1 if you have the `pthread' library (-lpthread). */
#ifndef THRIFT_HAVE_LIBPTHREAD
#define THRIFT_HAVE_LIBPTHREAD 1
#endif

/* Define to 1 if you have the `rt' library (-lrt). */
#ifndef THRIFT_HAVE_LIBRT
#define THRIFT_HAVE_LIBRT 1
#endif

/* Define to 1 if you have the `sasl2' library (-lsasl2). */
#ifndef THRIFT_HAVE_LIBSASL2
#define THRIFT_HAVE_LIBSASL2 1
#endif

/* Define to 1 if you have the `snappy' library (-lsnappy). */
#ifndef THRIFT_HAVE_LIBSNAPPY
#define THRIFT_HAVE_LIBSNAPPY 1
#endif

/* Define to 1 if you have the `socket' library (-lsocket). */
/* #undef HAVE_LIBSOCKET */

/* Define to 1 if you have the <limits.h> header file. */
#ifndef THRIFT_HAVE_LIMITS_H
#define THRIFT_HAVE_LIMITS_H 1
#endif

/* Define to 1 if your system has a GNU libc compatible `malloc' function, and
   to 0 otherwise. */
#ifndef THRIFT_HAVE_MALLOC
#define THRIFT_HAVE_MALLOC 1
#endif

/* Define to 1 if you have the <malloc.h> header file. */
#ifndef THRIFT_HAVE_MALLOC_H
#define THRIFT_HAVE_MALLOC_H 1
#endif

/* Define to 1 if you have the `memmove' function. */
#ifndef THRIFT_HAVE_MEMMOVE
#define THRIFT_HAVE_MEMMOVE 1
#endif

/* Define to 1 if you have the <memory.h> header file. */
#ifndef THRIFT_HAVE_MEMORY_H
#define THRIFT_HAVE_MEMORY_H 1
#endif

/* Define to 1 if you have the `memset' function. */
#ifndef THRIFT_HAVE_MEMSET
#define THRIFT_HAVE_MEMSET 1
#endif

/* Define to 1 if you have the `mkdir' function. */
#ifndef THRIFT_HAVE_MKDIR
#define THRIFT_HAVE_MKDIR 1
#endif

/* Define to 1 if you have the <netdb.h> header file. */
#ifndef THRIFT_HAVE_NETDB_H
#define THRIFT_HAVE_NETDB_H 1
#endif

/* Define to 1 if you have the <netinet/in.h> header file. */
#ifndef THRIFT_HAVE_NETINET_IN_H
#define THRIFT_HAVE_NETINET_IN_H 1
#endif

/* Define to 1 if you have the <openssl/rand.h> header file. */
#ifndef THRIFT_HAVE_OPENSSL_RAND_H
#define THRIFT_HAVE_OPENSSL_RAND_H 1
#endif

/* Define to 1 if you have the <openssl/ssl.h> header file. */
#ifndef THRIFT_HAVE_OPENSSL_SSL_H
#define THRIFT_HAVE_OPENSSL_SSL_H 1
#endif

/* Define to 1 if you have the <openssl/x509v3.h> header file. */
#ifndef THRIFT_HAVE_OPENSSL_X509V3_H
#define THRIFT_HAVE_OPENSSL_X509V3_H 1
#endif

/* Define to 1 if you have the <pthread.h> header file. */
#ifndef THRIFT_HAVE_PTHREAD_H
#define THRIFT_HAVE_PTHREAD_H 1
#endif

/* Define to 1 if the system has the type `ptrdiff_t'. */
#ifndef THRIFT_HAVE_PTRDIFF_T
#define THRIFT_HAVE_PTRDIFF_T 1
#endif

/* If available, contains the Python version number currently in use. */
#ifndef THRIFT_HAVE_PYTHON
#define THRIFT_HAVE_PYTHON "2.7"
#endif

/* Define to 1 if your system has a GNU libc compatible `realloc' function,
   and to 0 otherwise. */
#ifndef THRIFT_HAVE_REALLOC
#define THRIFT_HAVE_REALLOC 1
#endif

/* Define to 1 if you have the `realpath' function. */
#ifndef THRIFT_HAVE_REALPATH
#define THRIFT_HAVE_REALPATH 1
#endif

/* Define to 1 if you have the `sched_get_priority_max' function. */
#ifndef THRIFT_HAVE_SCHED_GET_PRIORITY_MAX
#define THRIFT_HAVE_SCHED_GET_PRIORITY_MAX 1
#endif

/* Define to 1 if you have the `sched_get_priority_min' function. */
#ifndef THRIFT_HAVE_SCHED_GET_PRIORITY_MIN
#define THRIFT_HAVE_SCHED_GET_PRIORITY_MIN 1
#endif

/* Define to 1 if you have the <sched.h> header file. */
#ifndef THRIFT_HAVE_SCHED_H
#define THRIFT_HAVE_SCHED_H 1
#endif

/* Define to 1 if you have the `select' function. */
#ifndef THRIFT_HAVE_SELECT
#define THRIFT_HAVE_SELECT 1
#endif

/* Define to 1 if you have the `socket' function. */
#ifndef THRIFT_HAVE_SOCKET
#define THRIFT_HAVE_SOCKET 1
#endif

/* Define to 1 if you have the `sqrt' function. */
#ifndef THRIFT_HAVE_SQRT
#define THRIFT_HAVE_SQRT 1
#endif

/* Define to 1 if `stat' has the bug that it succeeds when given the
   zero-length file name argument. */
/* #undef HAVE_STAT_EMPTY_STRING_BUG */

/* Define to 1 if stdbool.h conforms to C99. */
#ifndef THRIFT_HAVE_STDBOOL_H
#define THRIFT_HAVE_STDBOOL_H 1
#endif

/* Define if g++ supports C++0x features. */
#ifndef THRIFT_HAVE_STDCXX_0X
#define THRIFT_HAVE_STDCXX_0X /**/
#endif

/* Define to 1 if you have the <stddef.h> header file. */
#ifndef THRIFT_HAVE_STDDEF_H
#define THRIFT_HAVE_STDDEF_H 1
#endif

/* Define to 1 if you have the <stdint.h> header file. */
#ifndef THRIFT_HAVE_STDINT_H
#define THRIFT_HAVE_STDINT_H 1
#endif

/* Define to 1 if you have the <stdlib.h> header file. */
#ifndef THRIFT_HAVE_STDLIB_H
#define THRIFT_HAVE_STDLIB_H 1
#endif

/* Define to 1 if you have the `strchr' function. */
#ifndef THRIFT_HAVE_STRCHR
#define THRIFT_HAVE_STRCHR 1
#endif

/* Define to 1 if you have the `strdup' function. */
#ifndef THRIFT_HAVE_STRDUP
#define THRIFT_HAVE_STRDUP 1
#endif

/* Define to 1 if you have the `strerror' function. */
#ifndef THRIFT_HAVE_STRERROR
#define THRIFT_HAVE_STRERROR 1
#endif

/* Define to 1 if you have the `strerror_r' function. */
#ifndef THRIFT_HAVE_STRERROR_R
#define THRIFT_HAVE_STRERROR_R 1
#endif

/* Define to 1 if you have the `strftime' function. */
#ifndef THRIFT_HAVE_STRFTIME
#define THRIFT_HAVE_STRFTIME 1
#endif

/* Define to 1 if you have the <strings.h> header file. */
#ifndef THRIFT_HAVE_STRINGS_H
#define THRIFT_HAVE_STRINGS_H 1
#endif

/* Define to 1 if you have the <string.h> header file. */
#ifndef THRIFT_HAVE_STRING_H
#define THRIFT_HAVE_STRING_H 1
#endif

/* Define to 1 if you have the `strstr' function. */
#ifndef THRIFT_HAVE_STRSTR
#define THRIFT_HAVE_STRSTR 1
#endif

/* Define to 1 if you have the `strtol' function. */
#ifndef THRIFT_HAVE_STRTOL
#define THRIFT_HAVE_STRTOL 1
#endif

/* Define to 1 if you have the `strtoul' function. */
#ifndef THRIFT_HAVE_STRTOUL
#define THRIFT_HAVE_STRTOUL 1
#endif

/* Define to 1 if you have the <sys/param.h> header file. */
#ifndef THRIFT_HAVE_SYS_PARAM_H
#define THRIFT_HAVE_SYS_PARAM_H 1
#endif

/* Define to 1 if you have the <sys/poll.h> header file. */
#ifndef THRIFT_HAVE_SYS_POLL_H
#define THRIFT_HAVE_SYS_POLL_H 1
#endif

/* Define to 1 if you have the <sys/resource.h> header file. */
#ifndef THRIFT_HAVE_SYS_RESOURCE_H
#define THRIFT_HAVE_SYS_RESOURCE_H 1
#endif

/* Define to 1 if you have the <sys/select.h> header file. */
#ifndef THRIFT_HAVE_SYS_SELECT_H
#define THRIFT_HAVE_SYS_SELECT_H 1
#endif

/* Define to 1 if you have the <sys/socket.h> header file. */
#ifndef THRIFT_HAVE_SYS_SOCKET_H
#define THRIFT_HAVE_SYS_SOCKET_H 1
#endif

/* Define to 1 if you have the <sys/stat.h> header file. */
#ifndef THRIFT_HAVE_SYS_STAT_H
#define THRIFT_HAVE_SYS_STAT_H 1
#endif

/* Define to 1 if you have the <sys/time.h> header file. */
#ifndef THRIFT_HAVE_SYS_TIME_H
#define THRIFT_HAVE_SYS_TIME_H 1
#endif

/* Define to 1 if you have the <sys/types.h> header file. */
#ifndef THRIFT_HAVE_SYS_TYPES_H
#define THRIFT_HAVE_SYS_TYPES_H 1
#endif

/* Define to 1 if you have the <sys/un.h> header file. */
#ifndef THRIFT_HAVE_SYS_UN_H
#define THRIFT_HAVE_SYS_UN_H 1
#endif

/* Define to 1 if you have <sys/wait.h> that is POSIX.1 compatible. */
#ifndef THRIFT_HAVE_SYS_WAIT_H
#define THRIFT_HAVE_SYS_WAIT_H 1
#endif

/* Define to 1 if you have the <unistd.h> header file. */
#ifndef THRIFT_HAVE_UNISTD_H
#define THRIFT_HAVE_UNISTD_H 1
#endif

/* Define to 1 if you have the `vfork' function. */
#ifndef THRIFT_HAVE_VFORK
#define THRIFT_HAVE_VFORK 1
#endif

/* Define to 1 if you have the <vfork.h> header file. */
/* #undef HAVE_VFORK_H */

/* Define to 1 if you have the `vprintf' function. */
#ifndef THRIFT_HAVE_VPRINTF
#define THRIFT_HAVE_VPRINTF 1
#endif

/* Define to 1 if `fork' works. */
#ifndef THRIFT_HAVE_WORKING_FORK
#define THRIFT_HAVE_WORKING_FORK 1
#endif

/* Define to 1 if `vfork' works. */
#ifndef THRIFT_HAVE_WORKING_VFORK
#define THRIFT_HAVE_WORKING_VFORK 1
#endif

/* define if zlib is available */
#ifndef THRIFT_HAVE_ZLIB
#define THRIFT_HAVE_ZLIB /**/
#endif

/* Define to 1 if the system has the type `_Bool'. */
/* #undef HAVE__BOOL */

/* Possible value for SIGNED_RIGHT_SHIFT_IS */
#ifndef THRIFT_LOGICAL_RIGHT_SHIFT
#define THRIFT_LOGICAL_RIGHT_SHIFT 2
#endif

/* Define to 1 if `lstat' dereferences a symlink specified with a trailing
   slash. */
#ifndef THRIFT_LSTAT_FOLLOWS_SLASHED_SYMLINK
#define THRIFT_LSTAT_FOLLOWS_SLASHED_SYMLINK 1
#endif

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#ifndef THRIFT_LT_OBJDIR
#define THRIFT_LT_OBJDIR ".libs/"
#endif

/* Define to 1 if your C compiler doesn't accept -c and -o together. */
/* #undef NO_MINUS_C_MINUS_O */

/* Name of package */
#ifndef THRIFT_PACKAGE
#define THRIFT_PACKAGE "thrift"
#endif

/* Define to the address where bug reports for this package should be sent. */
#ifndef THRIFT_PACKAGE_BUGREPORT
#define THRIFT_PACKAGE_BUGREPORT ""
#endif

/* Define to the full name of this package. */
#ifndef THRIFT_PACKAGE_NAME
#define THRIFT_PACKAGE_NAME "thrift"
#endif

/* Define to the full name and version of this package. */
#ifndef THRIFT_PACKAGE_STRING
#define THRIFT_PACKAGE_STRING "thrift 1.0"
#endif

/* Define to the one symbol short name of this package. */
#ifndef THRIFT_PACKAGE_TARNAME
#define THRIFT_PACKAGE_TARNAME "thrift"
#endif

/* Define to the home page for this package. */
#ifndef THRIFT_PACKAGE_URL
#define THRIFT_PACKAGE_URL ""
#endif

/* Define to the version of this package. */
#ifndef THRIFT_PACKAGE_VERSION
#define THRIFT_PACKAGE_VERSION "1.0"
#endif

/* Define as the return type of signal handlers (`int' or `void'). */
#ifndef THRIFT_RETSIGTYPE
#define THRIFT_RETSIGTYPE void
#endif

/* Define to the type of arg 1 for `select'. */
#ifndef THRIFT_SELECT_TYPE_ARG1
#define THRIFT_SELECT_TYPE_ARG1 int
#endif

/* Define to the type of args 2, 3 and 4 for `select'. */
#ifndef THRIFT_SELECT_TYPE_ARG234
#define THRIFT_SELECT_TYPE_ARG234 (fd_set *)
#endif

/* Define to the type of arg 5 for `select'. */
#ifndef THRIFT_SELECT_TYPE_ARG5
#define THRIFT_SELECT_TYPE_ARG5 (struct timeval *)
#endif

/* Indicates the effect of the right shift operator on negative signed
   integers */
#ifndef THRIFT_SIGNED_RIGHT_SHIFT_IS
#define THRIFT_SIGNED_RIGHT_SHIFT_IS 1
#endif

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at runtime.
   STACK_DIRECTION > 0 => grows toward higher addresses
   STACK_DIRECTION < 0 => grows toward lower addresses
   STACK_DIRECTION = 0 => direction of growth unknown */
/* #undef STACK_DIRECTION */

/* Define to 1 if you have the ANSI C header files. */
#ifndef THRIFT_STDC_HEADERS
#define THRIFT_STDC_HEADERS 1
#endif

// https://developer.apple.com/library/mac/documentation/Darwin/Reference/ManPages/man3/strerror_r.3.html
// http://www.kernel.org/doc/man-pages/online/pages/man3/strerror.3.html
#if defined(__linux__) && !defined(__ANDROID__)
/* Define to 1 if strerror_r returns char *. */
#ifndef THRIFT_STRERROR_R_CHAR_P
#define THRIFT_STRERROR_R_CHAR_P 1
#endif
#endif // defined(__linux__) && !defined(__ANDROID__)

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#ifndef THRIFT_TIME_WITH_SYS_TIME
#define THRIFT_TIME_WITH_SYS_TIME 1
#endif

/* Define to 1 if your <sys/time.h> declares `struct tm'. */
/* #undef TM_IN_SYS_TIME */

/* Possible value for SIGNED_RIGHT_SHIFT_IS */
#ifndef THRIFT_UNKNOWN_RIGHT_SHIFT
#define THRIFT_UNKNOWN_RIGHT_SHIFT 3
#endif

/* experimental --enable-boostthreads that replaces POSIX pthread by
   boost::thread */
/* #undef USE_BOOST_THREAD */

/* Version number of package */
#ifndef THRIFT_VERSION
#define THRIFT_VERSION "1.0"
#endif

/* Define to 1 if `lex' declares `yytext' as a `char *' by default, not a
   `char[]'. */
#ifndef THRIFT_YYTEXT_POINTER
#define THRIFT_YYTEXT_POINTER 1
#endif

/* Define for Solaris 2.5.1 so the uint32_t typedef from <sys/synch.h>,
   <pthread.h>, or <semaphore.h> is not used. If the typedef were allowed, the
   #define below would cause a syntax error. */
/* #undef _UINT32_T */

/* Define for Solaris 2.5.1 so the uint64_t typedef from <sys/synch.h>,
   <pthread.h>, or <semaphore.h> is not used. If the typedef were allowed, the
   #define below would cause a syntax error. */
/* #undef _UINT64_T */

/* Define for Solaris 2.5.1 so the uint8_t typedef from <sys/synch.h>,
   <pthread.h>, or <semaphore.h> is not used. If the typedef were allowed, the
   #define below would cause a syntax error. */
/* #undef _UINT8_T */

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif

/* Define to the type of a signed integer type of width exactly 16 bits if
   such a type exists and the standard includes do not define it. */
/* #undef int16_t */

/* Define to the type of a signed integer type of width exactly 32 bits if
   such a type exists and the standard includes do not define it. */
/* #undef int32_t */

/* Define to the type of a signed integer type of width exactly 64 bits if
   such a type exists and the standard includes do not define it. */
/* #undef int64_t */

/* Define to the type of a signed integer type of width exactly 8 bits if such
   a type exists and the standard includes do not define it. */
/* #undef int8_t */

/* Define to rpl_malloc if the replacement function should be used. */
/* #undef malloc */

/* Define to `int' if <sys/types.h> does not define. */
/* #undef mode_t */

/* Define to `long int' if <sys/types.h> does not define. */
/* #undef off_t */

/* Define to `int' if <sys/types.h> does not define. */
/* #undef pid_t */

/* Define to rpl_realloc if the replacement function should be used. */
/* #undef realloc */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */

/* Define to `int' if <sys/types.h> does not define. */
/* #undef ssize_t */

/* Define to the type of an unsigned integer type of width exactly 16 bits if
   such a type exists and the standard includes do not define it. */
/* #undef uint16_t */

/* Define to the type of an unsigned integer type of width exactly 32 bits if
   such a type exists and the standard includes do not define it. */
/* #undef uint32_t */

/* Define to the type of an unsigned integer type of width exactly 64 bits if
   such a type exists and the standard includes do not define it. */
/* #undef uint64_t */

/* Define to the type of an unsigned integer type of width exactly 8 bits if
   such a type exists and the standard includes do not define it. */
/* #undef uint8_t */

/* Define as `fork' if `vfork' does not work. */
/* #undef vfork */

/* Define to empty if the keyword `volatile' does not work. Warning: valid
   code using `volatile' can become incorrect without. Disable with care. */
/* #undef volatile */

/* once: _LIB_CPP_THRIFT_CONFIG_H */
#endif
