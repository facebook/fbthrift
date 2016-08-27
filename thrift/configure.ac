#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements. See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership. The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License. You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the License for the
# specific language governing permissions and limitations
# under the License.
#

AC_PREREQ(2.65)

m4_define([thrift_version_str], m4_esyscmd_s([cat VERSION]))

AC_INIT([thrift], m4_translit(thrift_version_str, [:], [.]))

LT_VERSION=thrift_version_str:0
AC_SUBST([LT_VERSION])

AC_CONFIG_AUX_DIR([build-aux])

AM_INIT_AUTOMAKE([1.9 foreign tar-ustar nostdinc subdir-objects])
AC_CONFIG_MACRO_DIR([m4])

PKG_PROG_PKG_CONFIG

AC_ARG_VAR([PY_PREFIX], [Prefix for installing Python modules.
                         (Normal --prefix is ignored for Python because
                         Python has different conventions.)
                         Default = Python sys.prefix])

AC_ARG_VAR([PY_INSTALL_HOME], [Prefix for installing Python modules using
                               home scheme. Usually unused.])

AX_PYTHON_DEVEL()
AS_IF([test x"$PY_PREFIX" != x],
          [PY_INSTALL_ARGS="--prefix '$PY_PREFIX'"
           PY_CUSTOM_SITE_PKGS="$PY_PREFIX/lib/python$ac_python_version/site-packages"
           PY_RUN_ENV="PYTHONPATH=$PY_CUSTOM_SITE_PKGS"],
      [test x"$PY_INSTALL_HOME" != x],
          [PY_INSTALL_ARGS="--home '$PY_INSTALL_HOME'"
           PY_RUN_ENV="PYTHONPATH='$PY_INSTALL_HOME/lib/python'"])

AC_SUBST([PY_INSTALL_ARGS])
AC_SUBST([PY_CUSTOM_SITE_PKGS])
AC_SUBST([PY_RUN_ENV])

AC_ARG_VAR([DMD_LIBEVENT_FLAGS], [DMD flags for linking libevent (auto-detected if not set).])
AC_ARG_VAR([DMD_OPENSSL_FLAGS], [DMD flags for linking OpenSSL (auto-detected if not set).])

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CPP
AC_PROG_CXX
AC_CXX_COMPILE_STDCXX_0X
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_MAKE_SET
AC_PROG_YACC
AC_PROG_LEX
AM_PROG_LEX
AC_PROG_LN_S
AC_PROG_MKDIR_P
AC_PROG_AWK

AC_LANG([C++])

CXXFLAGS=""
if test "$ac_cv_cxx_compile_cxx0x_cxx" = yes; then
   CXXFLAGS="-std=c++1y"
fi
if test "$ac_cv_cxx_compile_cxx0x_gxx" = yes; then
   CXXFLAGS="-std=gnu++1y"
fi


AX_THRIFT_LIB(cpp, [C++], yes)
AX_BOOST_BASE([1.40.0])
if test "x$succeeded" != "xyes" ; then
  AC_MSG_ERROR([Please install libboost-dev-all])
fi
AX_BOOST_PYTHON

if test "x$LEX" != xflex; then
  AC_MSG_ERROR([Please install flex])
  have_cpp="no"
fi
case $YACC in bison*) ;;
  *) AC_MSG_ERROR([Please install bison])
     have_cpp="no" ;;
esac

AX_CHECK_FOLLY([], [AC_MSG_ERROR(
           [Please install the folly library])])
LDFLAGS="$LDFLAGS $FOLLY_LDFLAGS"
CPPFLAGS="$FOLLY_INCLUDES $CPPFLAGS"

AC_CHECK_LIB([glog],[openlog],[],[AC_MSG_ERROR(
           [Please install google-glog library])])

have_cpp=yes
have_libnuma=no
if test "$with_cpp" = "yes";  then
  AC_CHECK_LIB([gflags],[getenv],[],[AC_MSG_ERROR(
             [Please install google-gflags library])])
  AC_CHECK_LIB([snappy], [main], [], [AC_MSG_ERROR(
             [Please install snappy library])])
  AC_CHECK_LIB([wangle], [getenv], [], [AC_MSG_ERROR(
             [Please install the wangle library from https://github.com/facebook/wangle])])
  AC_CHECK_LIB([mstch], [getenv], [], [AC_MSG_ERROR(
             [Please install the mstch libarary from https://github.com/no1msd/mstch])])

  AC_CHECK_LIB([numa], [numa_available],[
    AC_CHECK_HEADER([numa.h], [
      have_libnuma=yes
    ], [
      AC_MSG_ERROR([numa not installed correctly])
    ])
  ])

  AX_BOOST_THREAD
  AX_BOOST_FILESYSTEM
  AX_BOOST_SYSTEM

  AX_LIB_EVENT([1.0])
  have_libevent=$success
  if test x"$success" != x"yes"; then
    AC_MSG_ERROR([Please install libevent])
  fi

  AC_CHECK_HEADER([gssapi/gssapi_generic.h], [], [
    AC_MSG_ERROR([Please install libkrb5-dev])
  ], [])

  AC_CHECK_LIB(sasl2, sasl_client_new, [], [
    AC_MSG_ERROR([Please install libsasl2])
  ], [])

  AC_CHECK_HEADER([snappy.h], [], [
    AC_MSG_ERROR([Please install libsnappy-dev])
  ], [])

  AC_CHECK_HEADERS([mstch/mstch.hpp], [], [
    AC_MSG_ERROR([Please install mstch])
  ], [])

  AX_LIB_ZLIB([1.2.3])
  have_zlib=$success
fi
AM_CONDITIONAL([WITH_CPP], [test "$have_cpp" = "yes" -a "$with_cpp" = "yes"])
AM_CONDITIONAL([AMX_HAVE_ZLIB], [test "$have_zlib" = "yes"])
AM_CONDITIONAL([WITH_NUMA], [test "$have_libnuma" = "yes"])

AX_THRIFT_LIB(d, [D], yes)
if test "$with_d" = "yes";  then
  AX_DMD
  AC_SUBST(DMD)
  if test "x$DMD" != "x"; then
    have_d="yes"
  fi
fi

# Determine actual name of the generated D library for use in the command line
# when compiling tests. This is needed because the -l<lib> syntax doesn't work
# with OPTLINK (Windows).
lib_prefix=lib
lib_suffix=a
case "$host_os" in
  cygwin* | mingw* | pw32* | cegcc*)
    lib_prefix=""
    lib_suffix=lib
    ;;
esac
D_LIB_NAME="${lib_prefix}thriftd.${lib_suffix}"
AC_SUBST(D_LIB_NAME)
D_EVENT_LIB_NAME="${lib_prefix}thriftd-event.${lib_suffix}"
AC_SUBST(D_EVENT_LIB_NAME)
D_SSL_LIB_NAME="${lib_prefix}thriftd-ssl.${lib_suffix}"
AC_SUBST(D_SSL_LIB_NAME)

if test "$have_d" = "yes"; then
  AX_CHECK_D_MODULE(deimos.event2.event)
  have_deimos_event2=$success

  with_d_event_tests="no"
  if test "$have_deimos_event2" = "yes"; then
    if test "x$DMD_LIBEVENT_FLAGS" = "x"; then
      if test "$dmd_optlink" = "yes"; then
        AC_MSG_WARN([D libevent interface found, but cannot auto-detect \
linker flags for OPTLINK. Please set DMD_LIBEVENT_FLAGS manually.])
      else
        AX_LIB_EVENT([2.0])
        if test "$success" = "yes"; then
          DMD_LIBEVENT_FLAGS=$(echo "$LIBEVENT_LDFLAGS $LIBEVENT_LIBS" | \
            sed -e 's/^ *//g;s/ *$//g;s/^\(.\)/-L\1/g;s/  */ -L/g')
          with_d_event_tests="yes"
        else
          AC_MSG_WARN([D libevent interface present, but libevent library not found.])
        fi
      fi
    else
      with_d_event_tests="yes"
    fi
  fi

  AX_CHECK_D_MODULE(deimos.openssl.ssl)
  have_deimos_openssl=$success

  with_d_ssl_tests="no"
  if test "$have_deimos_openssl" = "yes"; then
    if test "x$DMD_OPENSSL_FLAGS" = "x"; then
      if test "$dmd_optlink" = "yes"; then
        AC_MSG_WARN([D OpenSSL interface found, but cannot auto-detect \
linker flags for OPTLINK. Please set DMD_OPENSSL_FLAGS manually.])
      else
        AX_CHECK_OPENSSL([with_d_ssl_tests="yes"])
        if test "$with_d_ssl_tests" = "yes"; then
          DMD_OPENSSL_FLAGS=$(echo "$OPENSSL_LDFLAGS $OPENSSL_LIBS" | \
            sed -e 's/^ *//g;s/ *$//g;s/^\(.\)/-L\1/g;s/  */ -L/g')
        else
          AC_MSG_WARN([D OpenSSL interface present, but OpenSSL library not found.])
        fi
      fi
    else
      with_d_ssl_tests="yes"
    fi
  fi
fi

AM_CONDITIONAL(WITH_D, [test "$have_d" = "yes"])
AM_CONDITIONAL(DMD_OPTLINK, [test "$dmd_optlink" = "yes"])
AC_SUBST(DMD_OF_DIRSEP, "$dmd_of_dirsep")
AM_CONDITIONAL(HAVE_DEIMOS_EVENT2, [test "$have_deimos_event2" = "yes"])
AM_CONDITIONAL(WITH_D_EVENT_TESTS, [test "$with_d_event_tests" = "yes"])
AC_SUBST(DMD_LIBEVENT_FLAGS)
AM_CONDITIONAL(HAVE_DEIMOS_OPENSSL, [test "$have_deimos_openssl" = "yes"])
AM_CONDITIONAL(WITH_D_SSL_TESTS, [test "$with_d_ssl_tests" = "yes"])
AC_SUBST(DMD_OPENSSL_FLAGS)


AM_CONDITIONAL(MINGW, false)
case "${host_os}" in
*mingw*)
  mingw32_support="yes"
  AC_CHECK_HEADER(windows.h)
  AM_CONDITIONAL(MINGW, true)
  ;;
*)
  AC_ISC_POSIX
  ;;
esac

AC_C_CONST
AC_C_INLINE
AC_C_VOLATILE

AC_HEADER_STDBOOL
AC_HEADER_STDC
AC_HEADER_TIME
AC_HEADER_SYS_WAIT
AC_TYPE_SIGNAL
AC_CHECK_HEADERS([arpa/inet.h])
AC_CHECK_HEADERS([sys/param.h])
AC_CHECK_HEADERS([fcntl.h])
AC_CHECK_HEADERS([inttypes.h])
AC_CHECK_HEADERS([limits.h])
AC_CHECK_HEADERS([netdb.h])
AC_CHECK_HEADERS([netinet/in.h])
AC_CHECK_HEADERS([pthread.h])
AC_CHECK_HEADERS([stddef.h])
AC_CHECK_HEADERS([stdlib.h])
AC_CHECK_HEADERS([sys/socket.h])
AC_CHECK_HEADERS([sys/time.h])
AC_CHECK_HEADERS([sys/un.h])
AC_CHECK_HEADERS([sys/poll.h])
AC_CHECK_HEADERS([sys/resource.h])
AC_CHECK_HEADERS([unistd.h])
AC_CHECK_HEADERS([libintl.h])
AC_CHECK_HEADERS([malloc.h])
AC_CHECK_HEADERS([openssl/ssl.h])
AC_CHECK_HEADERS([openssl/rand.h])
AC_CHECK_HEADERS([openssl/x509v3.h])
AC_CHECK_HEADERS([sched.h])

AC_CHECK_LIB(pthread, pthread_create)
dnl NOTE(dreiss): I haven't been able to find any really solid docs
dnl on what librt is and how it fits into various Unix systems.
dnl My best guess is that it is where glibc stashes its implementation
dnl of the POSIX Real-Time Extensions.  This seems necessary on Linux,
dnl and we haven't yet found a system where this is a problem.
AC_CHECK_LIB(rt, clock_gettime)
AC_CHECK_LIB(socket, setsockopt)

if test "$have_cpp" = "yes" ; then
# mingw toolchain used to build "Thrift Compiler for Windows"
# does not support libcrypto, so we just check if we building the cpp library
AC_CHECK_LIB(crypto,
    BN_init,
    [AC_CHECK_LIB(ssl,
        SSL_ctrl,
        [LIBS="-lssl -lcrypto $LIBS"],
        [AC_MSG_ERROR(["Error: libssl required"])],
        -lcrypto
    )],
    [AC_MSG_ERROR(["Error: libcrypto required."])]
)
fi

AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t], [], [exit 1])

AC_STRUCT_TM

dnl NOTE(dreiss): AI_ADDRCONFIG is not defined on OpenBSD.
AC_CHECK_DECL([AI_ADDRCONFIG], [],
              [AC_DEFINE([AI_ADDRCONFIG], 0,
                         [Define if the AI_ADDRCONFIG symbol is unavailable])],
              [
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netdb.h>
])

AC_FUNC_ALLOCA
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STAT
AC_FUNC_STRERROR_R
AC_FUNC_STRFTIME
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([strtoul])
AC_CHECK_FUNCS([bzero])
AC_CHECK_FUNCS([ftruncate])
AC_CHECK_FUNCS([gethostbyname])
AC_CHECK_FUNCS([gettimeofday])
AC_CHECK_FUNCS([memmove])
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([mkdir])
AC_CHECK_FUNCS([realpath])
AC_CHECK_FUNCS([select])
AC_CHECK_FUNCS([socket])
AC_CHECK_FUNCS([strchr])
AC_CHECK_FUNCS([strdup])
AC_CHECK_FUNCS([strerror])
AC_CHECK_FUNCS([strstr])
AC_CHECK_FUNCS([strtol])
AC_CHECK_FUNCS([sqrt])
dnl The following functions are optional.
AC_CHECK_FUNCS([alarm])
AC_CHECK_FUNCS([sched_get_priority_min])
AC_CHECK_FUNCS([sched_get_priority_max])

if test "$cross_compiling" = "no" ; then
  AX_SIGNED_RIGHT_SHIFT
fi

dnl autoscan thinks we need this macro because we have a member function
dnl called "error".  Invoke the macro but don't run the check so autoscan
dnl thinks we are in the clear.  It's highly unlikely that we will ever
dnl actually use the function that this checks for.
if false ; then
  AC_FUNC_ERROR_AT_LINE
fi

# --- Coverage hooks ---


AC_SUBST(AM_CPPFLAGS, '-I$(top_srcdir)/..'" $CXX_FLAGS $BOOST_CPPFLAGS")

AC_SUBST(PY_LOCAL_ROOT, '$(top_builddir)/.python-local')
AC_SUBST(PY_LOCAL_PATH, '$(PY_LOCAL_ROOT)/lib/python')

AC_CONFIG_HEADERS(config.h:config.hin)
AX_PREFIX_CONFIG_H([lib/cpp/thrift_config.h], [thrift], [config.h])

AC_CONFIG_FILES([
  Makefile
  compiler/Makefile
  compiler/py/Makefile
  compiler/version.h
  lib/Makefile
  lib/thrift/Makefile
  lib/cpp/Makefile
  lib/cpp2/Makefile
  lib/cpp2/test/Makefile
])

AC_OUTPUT


echo
echo "$PACKAGE $VERSION"
echo
echo "Building C++ Library ......... : $have_cpp"

echo
echo "If something is missing that you think should be present,"
echo "please skim the output of configure to find the missing"
echo "component.  Details are present in config.log."
