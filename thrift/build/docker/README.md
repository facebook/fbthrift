Construction of Docker images uses script files located in Thrift project's build directory. So we need to get it first.

Create/goto to you working directory

```
mkdir src
cd src
```
Check out Thrift source repository

```
git clone https://github.com/facebook/fbthrift.git
```

Now run script that will build the Docker image using this directory as a source.
```
./fbthrift/thrift/build/docker/build.sh `pwd`/fbthrift
```

This script will create (it will take a while) two Docker images: `thrift_sys_image` which contains system dependencies like gcc-5 and `thrift_lib_image` which contains all libraries required to build Thrift.

Now we can start working on the container to do edit/compile cycles in the Thrift project.
```
docker run -it -v `pwd`/fbthrift:/fbthrift thrift_lib_image /bin/bash
```
After executing the previous command we can use the interactive shell running inside the container.
For example, we can build thrift using CMake:
```
cd /fbthrift
mkdir build
cd build
cmake ..
make
```

Since the host directory that contains the sources of Thrift is now accessible from both host machine and container, it is now possible to edit files on the host machine using any advanced/favorite editor; using the container to run build/tests only.
