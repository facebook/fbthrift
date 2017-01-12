# FbThrift Compiler

The Thrift compiler is a standalone binary to read and generate code for any proper *.thrift file.

## Downloading:
```
git clone https://github.com/facebook/fbthrift.git
```

## Dependencies:
- [Mustache](https://mustache.github.io/) (Logic-less templates)
```
mkdir fbthrift/external
pushd fbthrift/external
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

### Windows (MSVC)
- Install [Microsoft Visual Studio](https://www.visualstudio.com/vs/)
 - Make sure to select Visual C++ tools and the Windows SDK during installation
 - Otherwise, open Visual Studio File > New > Project > Online. Then, instal Visual C++ and Windows SDK
- Install [Microsoft Build Tools](https://www.microsoft.com/en-us/download/details.aspx?id=48159)
 - Add MSBuild to the PATH variable
 ```
 # Using Powershell (make sure path matches your install directory):
 [System.Environment]::SetEnvironmentVariable("PATH", "$env:Path;C\Program Files\MSBuild\14.0\Bin", [System.EnvironmentVariableTarget]::Machine)
 ```

- Install [CMake](http://www.cmake.org)
 - Download and install one of the latest Windows .msi file
 - During install, select: Make path available for all users or for this user
- Install [precompiled Boost](https://sourceforge.net/projects/boost/files/boost-binaries/), or compile it on your own
 - Make sure that Boost matches your machine architecture (32 or 64) and your MSVC version
 - After installing open the installation path an rename the directory 'lib<version>' -> 'lib'
 - Add C:\path\to\boost_1_<ver>_0 to your PATH variable
 ```
 # Using Powershell (make sure path matches your install directory):
 [System.Environment]::SetEnvironmentVariable("PATH", "$env:Path;C\local\boost_1_<ver>_0", [System.EnvironmentVariableTarget]::Machine)
 ```
- Install [precompiled OpenSSL](https://slproweb.com/products/Win32OpenSSL.html), or compile it on your own
 - Download the complete version (no Light) and make sure that OpenSSL matches your machine architecture (32 or 64)
 - Default install, it will add the path to your PATH variable
- Download [winflexbison.zip](https://sourceforge.net/projects/winflexbison/)
 - Create a directory where your boost_1_<ver>_0 directory is called 'win_flex_bison'(or any name you want to use)
 - Unzip and move win_flex.exe, win_bison.exe, and data/ to that directory
 - Add the directory to your PATH variable
 ```
 # Using Powershell (make sure path matches your install directory):
 [System.Environment]::SetEnvironmentVariable("PATH", "$env:Path;C\local\win_flex_bison", [System.EnvironmentVariableTarget]::Machine)
 ```

## Building

```
mkdir fbthrift/build
cd fbthrift/thrift
cmake ..
```

## Installing

This will create a `thrift-compiler` binary file inside the build directory.

### Linux or MacOSX
```
make install
```

### Windows (MSVC)
```
# Using PowerShell
msbuild .\INSTALL.vcxproj
```

## Usage

### Linux or MacOSX
```
./thrift-compiler --help
```

### Windows (MSVC)
```
# Using PowerShell
thrift-compiler.exe --help
```
