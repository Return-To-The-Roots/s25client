# Return To The Roots

"Return To The Roots" is a fan-project, which aims to renew the original The Settlers 2.

We aim to extend new features such as a multiplayer mode via internet as well as the support for modern hardware and several operating systems like Windows XP/Vista/Seven, Linux and MacOS X. Likewise we want to invent some smaller upgrades. Unfortunately it is necessary to rewrite the whole game, but we will stick to the original graphics and sounds, because they are still common and nice to be heard or seen.
So you will still need an original "The Settlers 2 Gold Edition" version to play Return To The Roots.

see more information on http://www.rttr.info

# Current Build Info

Travis CI: [![Travis CI Build Info](https://travis-ci.org/Return-To-The-Roots/s25client.svg?branch=master)](https://travis-ci.org/Return-To-The-Roots/s25client)

Appveyor: [![Appveyor CI Build Info](https://ci.appveyor.com/api/projects/status/ufw8v9mi80va1me7/branch/master?svg=true)](https://ci.appveyor.com/project/Flow86/s25client/branch/master)

Coverage:
[![Coverage Status](https://coveralls.io/repos/github/Return-To-The-Roots/s25client/badge.svg?branch=master)](https://coveralls.io/github/Return-To-The-Roots/s25client?branch=master)
 / 
[![codecov](https://codecov.io/gh/Return-To-The-Roots/s25client/branch/master/graph/badge.svg)](https://codecov.io/gh/Return-To-The-Roots/s25client)

# How to build

## On Linux or Darwin/MacOSX

### Prerequisite Linux:
- cmake
- git
- boost / libboost1.55-dev (i.e https://github.com/Return-To-The-Roots/contrib or http://www.boost.org/)
- libsdl1.2-dev
- libsdl-mixer1.2-dev
- licurl-dev (in libcurl4-openssl-dev)
- libbz2-dev
- libminiupnpc-dev (linux)
- liblua5.2-dev (linux, i.e from contrib)   
Most of them can be installed with the package manager.

### Prerequisite MacOSX:
 - cmake
 - git
 - boost
 - sdl
 - sdl_mixer
 - gettext (make sure it is in your path with e.g. `brew link --force gettext`)
 - miniupnpc
All of them can be installed via homebrew

### Steps:
```
git clone --recursive https://github.com/Return-To-The-Roots/s25client s25client
cd s25client/build
./cmake.sh --prefix=.
make
```

MacOSX defaults to XCode generator. If you don't have XCode installed, use `./cmake.sh --prefix=. --generator="Unix Makefiles"` instead.

Note that due to the use of submodules you always need to `git pull && git submodule update --init --recursive` to get the latest version.
(The `--init` and `--recursive` arguments are only required should we add *new* submodules to the existing set.)

## On Windows

### Prerequisites:
- cmake (i.e from https://github.com/Return-To-The-Roots/contrib or http://www.cmake.org/download/)
- boost (i.e from https://github.com/Return-To-The-Roots/contrib or http://www.boost.org/)
- Visual Studio (at least 2010, you can get 2015 community for free)
- Git Client (i.e TortoiseGit)

### Steps:
- Clone GIT Repository from https://github.com/Return-To-The-Roots/s25client
  - Using Git bash:
     ```
     git clone --recursive https://github.com/Return-To-The-Roots/s25client s25client
     ```

  - **OR** using TortoiseGit:
     - Rightclick -> "Git clone..."
     - Put in https://github.com/Return-To-The-Roots/s25client as URL
     - Select "Directory" to clone to
     - press OK
     - Rightclick on the newly created folder -> TortoiseGit-> Submodule Update
     - Make sure all modules are selected and "Initialize submodules (--init)" is checked
     - press OK
- Extract contrib/full-contrib-msvc.rar to contrib 
  (so that contrib/full-contrib-msvc/bin, contrib/full-contrib-msvc/include and contrib/full-contrib-msvc/lib exist)
- If you havent installed boost, install boost
  Fast Way:
  - extract boost-1.55.zip (i.e to contrib/boost, so that contrib/boost/bootstrap.bat exist)
  - run that "bootstrap.bat"
  - run created "b2.exe": this should build boost
    Notice: if you have multiple Visual Studio versions installed, use the latest one with the "toolset"-parameter. 
    i.e "b2 toolset=msvc-14.0" for Visual Studio 2015
  - run "b2 install --prefix=%CD%": this should install everything so the system can find it
    Notice: Don't forget to add the toolset parameter if you already used it before
- Use cmake-gui:
  - "Where is the source code": Select checked out directory
  - "Where to build the binaries": Select "build" directory
  - Press configure
  - Select your compiler version (i.e Visual Studio 2010 x86)
  - If it can't find boost:
    - "Add Entry"
    - Enter as "Name" "BOOST_ROOT" (exact casing!)
    - Select for "Type" "PATH"
    - Enter boost installation path for "Value"
    - Press ok
  - Press generate
- Open and use build/s25client.sln

--

for advanced info or help see INSTALL file the [FAQ in the wiki](
 https://github.com/Return-To-The-Roots/s25client/wiki/%5BFAQ%5D-Compiling) or http://www.rttr.info 
