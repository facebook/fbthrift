# FbThrift Compiler

The Thrift compiler is a standalone binary to read and generate code for any proper *.thrift file.

## Downloading:
```
git clone https://github.com/facebook/fbthrift.git
```

## Dependencies:
- [Mustache](https://mustache.github.io/) (Logic-less templates)
```
mkdir fbthrift/thrift/compiler/external
pushd fbthrift/thrift/compiler/external
git clone https://github.com/no1msd/mstch
popd
```

- [Cmake](https://cmake.org/) package builder
- C++ [Boost](http://www.boost.org/) (On MacOSX version has to be between [1.54](http://www.boost.org/doc/libs/1_54_0/doc/html/quickbook/install.html) and [1.61](http://www.boost.org/doc/libs/1_61_0/doc/html/quickbook/install.html))
- [Flex](https://github.com/westes/flex) and [Bison](https://www.gnu.org/software/bison/)

### Ubuntu:
```
sudo apt-get install \
  cmake \
  flex \
  bison \
  libboost-all-dev
```

### MacOSX
Using [Homebrew](http://brew.sh/)
```
brew install \
  cmake \
  flex \
  bison \
  boost155
```

### Windows (MinGW)
- Install [MinGW with Boost](http://www.nuwen.net/mingw.html) in C:\MinGW
 - Add MinGW to the PATH variable.
 ```
 # Using PowerShell:
 [System.Environment]::SetEnvironmentVariable("PATH", "$env:Path;C\MinGW\bin", [System.EnvironmentVariableTarget]::Machine)
 ```
 - Otherwise, make sure any MinGW and Boost paths are set in the PATH variable.
- Install [CMake](http://www.cmake.org)
 - Download and install one of the latest Windows .msi file.
 - During install, select: Make path available for all users or for this user.
- Download [winflexbison.zip](https://sourceforge.net/projects/winflexbison/)
 - Unzip and move win_flex.exe, win_bison.exe, and data/ to: C:\MinGW\bin
 - Or, move win_flex_bison to any directory of your choice and add it to the PATH variable

## Building

### Linux or MacOS
This will create a single `thrift` binary file.
```
cd fbthrift/thrift
cmake ./compiler
make install
```

### Windows
This will create a single `thrift.exe` executable file.
Using PowerShell
```
# Using PowerShell:
cd fbthrift/thrift
cmake -DCMAKE_INSTALL_PREFIX:PATH=\. -G"MinGW Makefiles" .\compiler
make install
```

## Usage
```
./thrift --help
```
