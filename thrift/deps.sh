sudo apt-get install git autoconf libtool g++ libboost-all-dev libevent-dev flex bison libgoogle-glog-dev scons libkrb5-dev libsnappy-dev libsasl2-dev

git clone https://github.com/facebook/folly
cd folly/folly

wget --no-check-certificate https://double-conversion.googlecode.com/files/double-conversion-1.1.1.tar.gz
tar zxvf double-conversion-1.1.1.tar.gz
mv SConstruct.double-conversion double-conversion
cd double-conversion
scons -f SConstruct.double-conversion
cd ..

cd test
wget http://googletest.googlecode.com/files/gtest-1.6.0.zip
unzip gtest-1.6.0.zip
cd ..

autoreconf --install
LDFLAGS=-L`pwd`/double-conversion/ CPPFLAGS=-I`pwd`/double-conversion/src/ ./configure
make
make install 
cd ../..

autoreconf --install
CPPFLAGS="-I`pwd`/folly/folly/test/gtest-1.6.0/include/ -I`pwd`/folly/ -I`pwd`/folly/folly/double-conversion/src/" LDFLAGS="-L`pwd`/folly/folly/test/.libs/ -L`pwd`/folly/folly/.libs/ -L`pwd`/folly/folly/double-conversion/" ./configure
make
make check
