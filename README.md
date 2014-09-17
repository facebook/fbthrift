FBThrift: Facebook's branch of apache thrift
--------------------------------------------

The main focus of this package is the new C++ server, under thrift/lib/cpp2.  This repo also contains a branch of the rest of apache thrift's repo with any changes Facebook has made, however the build system only supports cpp2.

Apache thrift is at http://thrift.apache.org/

Building
--------

Note that under GCC, you probably need at least 2GB of memory to compile fbthrift.  If you see 'internal compiler error', this is probably because you ran out of memory during compilation.

Dependencies
------------

 - Facebook's folly library: http://www.github.com/facebook/folly

 - In addition to the packages required for building folly, Ubuntu 13.10 and
   14.04 require the following packages (feel free to cut and paste the apt-get
   command below):

```
  sudo apt-get install \
      flex \
      bison \
      libkrb5-dev \
      libsasl2-dev \
      libnuma-dev \
      pkg-config \
      libssl-dev
```

For your convenience, a build script is provided for ubuntu 14.04 64-bit:

```sh
cd fbthrift/thrift
./deps.sh
```

It will automatically pull down folly and build it, and then configure and build thrift.

 - Ubuntu 14.04 64-bit requires the following packages:

    - make
    - autoconf
    - libtool
    - g++
    - libboost-dev-all
    - libevent-dev
    - flex
    - bison
    - libgoogle-glog-dev
    - libdouble-conversion-dev
    - scons
    - libkrb5-dev
    - libsnappy-dev
    - libsasl2-dev

Docs
----

Some docs on the new cpp2 server are at:
https://github.com/facebook/fbthrift/blob/master/thrift/doc/Cpp2.md
