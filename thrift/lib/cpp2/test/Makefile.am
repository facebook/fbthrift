#
# Common thrift code generation rules
#
THRIFT = $(top_builddir)/compiler/thrift1

testutildir = $(includedir)/thrift/lib/cpp2/test/util

testutil_HEADERS = \
	util/TestInterface.h \
	util/TestServerFactory.h \
	util/TestThriftServerFactory.h

check_LIBRARIES = libService.a

gen-cpp2/Service_constants.cpp: Service.thrift
	PYTHONPATH=$(PY_LOCAL_PATH) python -mthrift_compiler.main --gen cpp2:json $<
	$(THRIFT) --gen cpp:templates,cob_style -r $<
gen-cpp2/Service_types.cpp: gen-cpp2/Service_constants.cpp
gen-cpp2/TestService.cpp: gen-cpp2/Service_constants.cpp

libService_a_SOURCES = gen-cpp2/Service_constants.cpp \
		       gen-cpp2/Service_data.cpp \
		       gen-cpp2/Service_types.cpp \
		       gen-cpp2/TestService.cpp \
		       gen-cpp2/TestService_client.cpp \
                       gen-cpp/Service_reflection.cpp

libService_a_CPPFLAGS = $(AM_CPPFLAGS) $(LIBEVENT_CPPFLAGS) -I../../cpp -I$(top_builddir)/../../gperftools-2.0.99/src

check_PROGRAMS = ThriftServerTest SharedServerTest

ThriftServerTest_SOURCES = util/ThriftServerTest.cpp
ThriftServerTest_LDADD = ../libthriftprotocol.la ../libthriftcpp2.la ../libsaslstubs.a libService.a ../../cpp/libthrift.la -levent -lkrb5 -lsnappy -lsasl2 -lfolly  -lgssapi_krb5 $(BOOST_THREAD_LIB) -ldouble-conversion
ThriftServerTest_LDFLAGS = -lglog -lgtest
ThriftServerTest_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_builddir)/../../gperftools-2.0.99/src -I../../../../

SharedServerTest_SOURCES = util/SharedServerTest.cpp
SharedServerTest_LDADD = ../libthriftprotocol.la ../libthriftcpp2.la ../libsaslstubs.a libService.a ../../cpp/libthrift.la -levent -lkrb5 -lsnappy -lsasl2 -lfolly  -lgssapi_krb5 $(BOOST_THREAD_LIB) -ldouble-conversion
SharedServerTest_LDFLAGS = -lglog -lgtest
SharedServerTest_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_builddir)/../../gperftools-2.0.99/src -I../../../../

TESTS = ThriftServerTest SharedServerTest

clean-local:
	rm -rf gen-cpp2/Service*
	rm -rf InstalledCheck

gen-cpp2/InstalledCheck_types.cpp: InstalledCheck.thrift
	$(PY_RUN_ENV) $(PYTHON) -mthrift_compiler.main --gen cpp2 InstalledCheck.thrift

InstalledCheck: gen-cpp2/TestService.cpp
	$(CXX) $(CXXFLAGS) -o InstalledCheck $(LDFLAGS) -L@prefix@/lib $(CPPFLAGS) -I@prefix@/include -I../../../../ InstalledCheck.cpp util/TestInterface.cpp gen-cpp2/InstalledCheck_types.cpp gen-cpp2/TestService.cpp gen-cpp2/TestService_client.cpp -lthriftprotocol -lthriftcpp2 -lthrift -lfolly -lglog $(BOOST_SYSTEM_LIB) $(BOOST_THREAD_LIB) -lpthread -lm -ldouble-conversion -lssl -lcrypto

installcheck: gen-cpp2/InstalledCheck_types.cpp InstalledCheck
