# Return To The Roots

"Return To The Roots" is a fan-project, which aims to renew the original The Settlers 2.

We aim to extend new features such as a multiplayer mode via internet as well as the support for modern hardware and several operating systems like Windows XP/Vista/Seven, Linux and MacOS. Likewise we want to invent some smaller upgrades. Unfortunately it is necessary to rewrite the whole game, but we will stick to the original graphics and sounds, because they are still common and nice to be heard or seen.
So you will still need an original "The Settlers 2 Gold Edition" version to play Return To The Roots.

see more information on http://www.rttr.info

# How to build

## On Linux or Darwin/MacOS

### Prerequisite:
- cmake
- git
- miniupnp-dev (linux)
- liblua52-dev (linux)

### Steps:
```
git clone https://github.com/Return-To-The-Roots/s25client s25client
cd s25client
git submodule update --init
cd build
./cmake.sh --prefix=.
make
```

## On Windows

### Prerequisite:
- cmake (from contrib)
- Visual Studio (at least 2010)
- Git Client (i.e TortoiseGit)

### Steps:
- Clone GIT Repository from https://github.com/Return-To-The-Roots/s25client
- Update/Initialize GIT Submodule
- Use cmake-gui:
  - "Where is the source code": Select checked out directory
  - "Where to build the binaries": Select "build" directory
  - ... TODO (SDL...)
  - Press configure
  - Press generate

--

for more info or help see INSTALL file or http://www.rttr.info 
