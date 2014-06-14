#
# Common thrift code generation rules
#
THRIFT = $(top_builddir)/compiler/thrift1
PYPATH = $(top_builddir)/compiler/py/build/lib.linux-x86_64-2.7:$(top_builddir)/compiler/py/build/lib

noinst_LIBRARIES = libService.a

gen-cpp2/Service_constants.cpp: Service.thrift
	PYTHONPATH=$(PYPATH) python $(top_builddir)/compiler/main.py --gen cpp2 $<
	$(THRIFT) --gen cpp:templates,cob_style -r $<

libService_a_SOURCES = gen-cpp2/Service_constants.cpp \
		       gen-cpp2/Service_types.cpp \
		       gen-cpp2/TestService.cpp \
                       gen-cpp/Service_reflection.cpp

libService_a_CPPFLAGS = $(AM_CPPFLAGS) $(LIBEVENT_CPPFLAGS) -I../../cpp -I$(top_builddir)/../../gperftools-2.0.99/src

check_PROGRAMS = ThriftServerTest

ThriftServerTest_SOURCES = ThriftServerTest.cpp
ThriftServerTest_LDADD = ../libthriftcpp2.la ../libsaslstubs.a libService.a ../../cpp/libthrift.la -levent -lkrb5 -lsnappy -lsasl2 -lfolly  -lgssapi_krb5 $(BOOST_THREAD_LIB) -lboost_thread 
ThriftServerTest_LDFLAGS = -lglog -lgtest 
ThriftServerTest_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_builddir)/../../gperftools-2.0.99/src

TESTS = ThriftServerTest

clean:
	rm -rf gen-cpp2
