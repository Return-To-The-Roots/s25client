# Return To The Roots

"Return To The Roots" is a fan-project, which aims to renew the original The Settlers 2.

We aim to extend new features such as a multiplayer mode via internet as well as the support for modern hardware and several operating systems like Windows XP/Vista/Seven, Linux and MacOS X. Likewise we want to invent some smaller upgrades. Unfortunately it is necessary to rewrite the whole game, but we will stick to the original graphics and sounds, because they are still common and nice to be heard or seen.
So you will still need an original "The Settlers 2 Gold Edition" version to play Return To The Roots.

see more information on http://www.rttr.info

# How to build

## On Linux or Darwin/MacOSX

### Prerequisite:
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

### Steps:
```
git clone --recursive https://github.com/Return-To-The-Roots/s25client s25client
cd s25client/build
./cmake.sh --prefix=.
make
```

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
  - run created "b2.exe" (this should build boost)
  - run "b2 install --prefix=%CD%" (this should install everything so the system can find it)
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
