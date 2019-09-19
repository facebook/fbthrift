include(FBPythonBinary)

set(
  THRIFT_PY_SRCS
  "TMultiplexedProcessor.py"
  "Thrift.py"
  "__init__.py"
  "protocol/TBinaryProtocol.py"
  "protocol/TCompactProtocol.py"
  "protocol/THeaderProtocol.py"
  "protocol/TJSONProtocol.py"
  "protocol/TMultiplexedProtocol.py"
  "protocol/TProtocol.py"
  "protocol/TProtocolDecorator.py"
  "protocol/TSimpleJSONProtocol.py"
  "server/TNonblockingServer.py"
  "server/TServer.py"
  "transport/THeaderTransport.py"
  "transport/TSSLSocket.py"
  "transport/TSocket.py"
  "transport/TTransport.py"
  "util/BytesStrIO.py"
  "util/Decorators.py"
  "util/Recursive.py"
  "util/Serializer.py"
  "util/TValidator.py"
  "util/__init__.py"
  "util/async_common.py"
  "util/fuzzer.py"
  "util/randomizer.py"
  "util/remote.py"
)
# thrift_py has external dependencies on the "six" and "future" python
# packages.  It will use the "snappy" and "zstd" compression packages if they
# are available, but does not require them.
add_fb_python_library(
  thrift_py
  SOURCES ${THRIFT_PY_SRCS}
  NAMESPACE "thrift"
  DEPENDS python-six::python-six
)
install_fb_python_library(
  thrift_py
  EXPORT fbthrift-exports
)

add_fb_python_library(
  thrift_py_inspect
  SOURCES "util/inspect.py"
  NAMESPACE "thrift"
)
install_fb_python_library(
  thrift_py_inspect
  EXPORT fbthrift-exports
)
