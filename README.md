# Return To The Roots

"Return To The Roots" is a fan-project, which aims to renew the original The Settlers 2.

We aim to extend new features such as a multiplayer mode via internet as well as the support for modern hardware and several operating systems like Windows Vista/Seven/10, Linux and MacOS X.
Likewise we want to invent some smaller upgrades.
Unfortunately it is necessary to rewrite the whole game, but we will stick to the original graphics and sounds, because they are still common and nice to be heard or seen.
So you will still need an original "The Settlers 2 Gold Edition" version to play Return To The Roots.

See more information on <http://www.rttr.info>

## Current Build Info

Build & Tests:
 [![Appveyor CI Build Info](https://ci.appveyor.com/api/projects/status/ufw8v9mi80va1me7/branch/master?svg=true)](https://ci.appveyor.com/project/Flow86/s25client/branch/master)
 ![GHA Unit tests](https://github.com/Return-To-The-Roots/s25client/workflows/Unit%20tests/badge.svg)
 ![Static analysis](https://github.com/Return-To-The-Roots/s25client/workflows/Static%20analysis/badge.svg)

Coverage:
[![Coverage Status Coveralls](https://coveralls.io/repos/github/Return-To-The-Roots/s25client/badge.svg?branch=master)](https://coveralls.io/github/Return-To-The-Roots/s25client?branch=master)
 /
[![Coverage Status Codecov](https://codecov.io/gh/Return-To-The-Roots/s25client/branch/master/graph/badge.svg)](https://codecov.io/gh/Return-To-The-Roots/s25client)

## How to install

- Download the game for your OS at [Downloads](https://www.rttr.info/index.php?com=dynamic&mod=2)
  - stable: Usually more stable
  - nightly: Latest features and bug fixes, but might be broken sometimes
- Extract into a folder of your choice
- Locate the file `put your S2-Installation in here` in that folder (usually at the root or in S2)
- Copy the DATA and GFX folder from the original The Settlers II Gold into the folder containing the above file
- Start `rttr.bat`/`rttr.sh` or the bundle (OSX only) to auto-update and start the game
  - Alternatively start `s25client` directly, but updates and music might be missing
- WARNING: Do not use symlinks/junction points/... for subfolders of your installation.
Putting RttR in a symlinked folder should work though.

## How to build

### On Linux or Darwin/MacOSX

#### Prerequisite Linux

- C++14 compatible compiler (e.g. GCC-6)
- cmake
- git
- libboost-dev (at least v1.69.0, i.e <http://www.boost.org/>)
  or only: libboost-test-dev libboost-locale-dev, libboost-iostreams-dev, libboost-filesystem-dev, libboost-program-options-dev (at least v1.69.0)
- libsdl2-dev
- libsdl2-mixer-dev
- libcurl-dev (in libcurl4-openssl-dev)
- libbz2-dev
- lua5.2-dev
- gettext
- libminiupnpc-dev

All of them can be installed with the package manager.

#### Prerequisite MacOSX

- cmake
- git
- boost
- sdl2
- sdl2_mixer
- gettext (make sure it is in your path with e.g. `brew link --force gettext`)
- miniupnpc

All of them can be installed via homebrew

#### Prerequisites with Nix

Nix users can open a nix-shell to get a development environment with all packages ready.

#### Checkout and build

```bash
git clone --recursive https://github.com/Return-To-The-Roots/s25client s25client
cd s25client
nix-shell # Optional, for Nix users only
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

Note: by using the `-G` option of `cmake` you can specify a generator, e.g. `cmake -G Xcode -DCMAKE_BUILD_TYPE=Release ..` will generate an Xcode project.
Please check `cmake --help` for more options.

#### Optimizations

There are various CMake options to control the build and optimization including ARM (Raspberry PI etc.) related ones.
Examples:

- RTTR_ENABLE_OPTIMIZATIONS/RTTR_ENABLE_WERROR
- RTTR_OPTIMIZATION_VECTOR_EXT (Non-Windows x86/x64 only)
- RTTR_OPTIMIZATION_TUNE (Non-Windows only)
- RTTR_TARGET_BOARD (ARM only)
See the description in CMake-GUI/ccmake for details.

Note that due to the use of submodules you always need to `git pull && git submodule update --init --recursive` to get the latest version.
(The `--init` and `--recursive` arguments are only required should we add *new* submodules to the existing set.)

#### Tests

Especially for developing you should build in Debug mode (`-DCMAKE_BUILD_TYPE=Debug`) and run the tests after executing `make` via `make test` or `ctest --output-on-failure`.
There is also an option to enable checks for undefined behavior (UBSAN) and memory errors (ASAN) like use-after-free or leaks.
Just pass `-DRTTR_ENABLE_SANITIZERS=ON` to CMake and use a recent GCC or Clang compiler to build.
Then just run (tests or application) as usual.

**Note**: Boost.Endian < 1.67 is known to have UB so use at least 1.67 when running the sanitizers.

### On Windows

#### Prerequisites

- cmake (i.e from <http://www.cmake.org/download/>)
- boost (i.e from <http://www.boost.org/>)
- Visual Studio (at least 2017, you can get the community edition for free)
- Git Client (i.e TortoiseGit)

#### Steps

- Clone GIT Repository from <https://github.com/Return-To-The-Roots/s25client>
  - Using Git bash:

     ```bash
     git clone --recursive https://github.com/Return-To-The-Roots/s25client s25client
     ```

  - **OR** using TortoiseGit:
    - Rightclick -> "Git clone..."
    - Put in <https://github.com/Return-To-The-Roots/s25client> as URL
    - Select "Directory" to clone to
    - press OK
    - Rightclick on the newly created folder -> TortoiseGit-> Submodule Update
    - Make sure all modules are selected and "Initialize submodules (--init)" is checked
    - press OK
- If you haven't installed boost, install boost
  Fast Way:
  - extract boost-1.69.zip (i.e to external/boost, so that external/boost/bootstrap.bat exist)
  - run that "bootstrap.bat"
  - run created "b2.exe": this should build boost
    Notice: if you have multiple Visual Studio versions installed, use the latest one with the "toolset"-parameter.
    i.e "b2 toolset=msvc-14.0" for Visual Studio 2015
  - run "b2 install --prefix=%CD%": this should install everything so the system can find it
    Notice: Don't forget to add the toolset parameter if you already used it before
- Use cmake-gui:
  - "Where is the source code": Select checked out directory
  - "Where to build the binaries": Select "build" directory (create if required)
  - Press configure
  - Select your compiler version (i.e Visual Studio 2015 x64)
  - If it can't find boost:
    - "Add Entry"
    - Enter as "Name" "BOOST_ROOT" (exact casing!)
    - Select for "Type" "PATH"
    - Enter boost installation path for "Value"
    - Press ok
  - Press generate
- Open and use build/s25client.sln

--

For advanced info or help see [FAQ in the wiki](https://github.com/Return-To-The-Roots/s25client/wiki/How-to-install-RttR) or <http://www.rttr.info>
