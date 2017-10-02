Facebook Thrift [![Build Status](https://travis-ci.org/facebook/fbthrift.svg?branch=master)](https://travis-ci.org/facebook/fbthrift)
--------------------------------------------

Thrift is a serialization and RPC framework for service communication. Thrift enables these features in all major languages, and there is strong support for C++, Python, Hack, and Java. Most services at Facebook are written using Thrift for RPC, and some storage systems use Thrift for serializing records on disk.

At a high level, Thrift is three major things:

### A Code Generator

Thrift has a code generator which generates data structures that can be serialized using Thrift, and client and server stubs for RPC, in different languages.

### A Serialization Framework

Thrift has a set of protocols for serialization that may be used in different languages to serialize the generated structures created from the code generator.

### An RPC Framework

Thrift has a framework to frame messages to send between clients and servers, and to call application-defined functions when receiving messages in different languages.

There are several key goals for these components:
* Ease of use
  Thrift takes care of the boilerplate of serialization and RPC, and enables the developer to focus on the schema of the system's serializable types and on the interfaces of system's RPC services.

* Cross language support
  Thrift enables intercommunication between different languages. For example, a Python client communicating with a C++ server.

* Performance
  Thrift structures and services enable fast serialization and deserialization, and its RPC protocol and frameworks are designed with performance as a feature.

* Backwards compatibility
  Thrift allows fields to be added to and removed from serializable types in a manner that preserves backward and forward compatibility.

## Building

### Dependencies
Please install the following dependencies before building Facebook Thrift:

**System**: [Flex](https://www.gnu.org/software/flex), [Bison](https://www.gnu.org/software/bison), [Krb5](https://web.mit.edu/kerberos), [Zlib](https://zlib.net), [PThreads](https://computing.llnl.gov/tutorials/pthreads). *MacOSX*: [OpenSSLv1.0.2g](https://www.openssl.org)

**External**: [Double Conversion](https://github.com/google/double-conversion), [GFlags](https://github.com/gflags/gflags), [GLog](https://github.com/google/glog), [Mstch](https://github.com/no1msd/mstch)

**Facebook**: [Folly](https://github.com/facebook/folly), [Wangle](https://github.com/facebook/wangle), [Zstd](https://github.com/facebook/zstd)

### Build
    git clone https://github.com/facebook/fbthrift
    cd build
    cmake .. # Add -DOPENSSL_ROOT_DIR for MacOSX. Usually in /usr/local/ssl
    make # or make -j $(nproc), or make install.

This will create:
  * thrift/bin/thrift1: The Thrift compiler binary to generate client and server code.
  * thrift/lib/libthriftcpp2.so: Runtime library for clients and servers.

### Thrift Files
When using thrift and the CMake build system, include: `ThriftLibrary.cmake` in your project. This includes the following macro to help when building Thrift files:

    thrift_library(
      #file_name
      #services
      #language
      #options
      #file_path
      #output_path
    )

This generates a library called: `file_name-language`. That is, for `Test.thrift` compiled as cpp2, it will generate the library `Test-cpp2`. This should be added as a dependency to any `*.h` or `*.cpp` file that contains an include to generated code.

## C++ Static Reflection
Information regarding C++ Static Reflection support can be found under the [static reflection library directory](thrift/lib/cpp2/fatal/), in the corresponding [`README` file](thrift/lib/cpp2/fatal/README.md).
