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
- boost / libboost1.55-dev (i.e from contrib)
- libminiupnpc-dev (linux)
- liblua5.2-dev (linux, i.e from contrib)

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

### Prerequisites:
- cmake (i.e from https://github.com/Return-To-The-Roots/contrib or http://www.cmake.org/download/)
- boost (i.e from https://github.com/Return-To-The-Roots/contrib or http://www.boost.org/)
- Visual Studio (at least 2010)
- Git Client (i.e TortoiseGit)

### Steps:
- Clone GIT Repository from https://github.com/Return-To-The-Roots/s25client
- And Update/Initialize GIT Submodule
	1. Using Git bash:
	
	```
	git clone https://github.com/Return-To-The-Roots/s25client s25client
	cd s25client
	git submodule update --init
	```
	
	2. **OR** using TortoiseGit:
		- Rightclick -> "Git clone..."
		- Put in https://github.com/Return-To-The-Roots/s25client as URL
		- Select "Directory" to clone to and push OK
		- Open the selected folder and rightclick in some free space -> TortoiseGit-> Submodule Update
		- Make sure you see some pathes and push OK
- Extract contrib/full-contrib-msvc2010.rar to contrib 
  (so that contrib/full-contrib-msvc2010/bin, contrib/full-contrib-msvc2010/include and contrib/full-contrib-msvc2010/lib exist)
- If you havent installed boost, install boost
  Fast Way:
  - extract boost-1.55.zip (i.e to contrib/boost, so that contrib/boost/bootstrap.bat exist)
  - run that "bootstrap.bat"
  - run created "b2.exe" (this should build boost)
- Use cmake-gui:
  - "Where is the source code": Select checked out directory
  - "Where to build the binaries": Select "build" directory
  - Press configure
  - If it can't find boost:
    - "Add Entry"
    - Enter as "Name" "BOOST_ROOT" (exact casing!)
    - Select for "Type" "PATH"
    - Enter boost installation path for "Value"
    - Press ok
  - Press generate
- Open and use build/s25client.sln

--

for advanced info or help see INSTALL file or http://www.rttr.info 
