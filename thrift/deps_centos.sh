sudo yum install -y \
    openssl-devel \
    openssl-libs \
    make \
    zip \
    git \
    autoconf \
    libtool \
    gcc-c++ \
    boost \
    libevent-devel \
    libevent \
    flex \
    bison \
    scons \
    krb5-devel \
    snappy-devel \
    libgsasl-devel \
    numactl-devel \
    numactl-libs

# no rpm for this?
if [ ! -e double-conversion ]; then
echo "Fetching double-conversion from git (yum failed)"
    git clone https://github.com/floitsch/double-conversion.git double-conversion
    cd double-conversion
    cmake . -DBUILD_SHARED_LIBS=ON
    sudo make install
    cd ..
fi

git clone https://github.com/facebook/folly
cd folly/folly

autoreconf --install
./configure
make -j8
cd ../..

autoreconf --install
$LD_LIBRARY_PATH_SAVED=$LD_LIBRARY_PATH
export LD_LIBRARY_PATH="`pwd`/folly/folly/.libs/:$LD_LIBRARY_PATH"
CPPFLAGS=" -I`pwd`/folly/" LDFLAGS="-L`pwd`/folly/folly/.libs/" ./configure
make -j8
