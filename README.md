# Return To The Roots

"Return To The Roots" is a fan-project, which aims to renew the original The Settlers 2.

We aim to extend new features such as a multiplayer mode via internet as well as the support for modern hardware and several operating systems like Windows XP/Vista/Seven, Linux and MacOS. Likewise we want to invent some smaller upgrades. Unfortunately it is necessary to rewrite the whole game, but we will stick to the original graphics and sounds, because they are still common and nice to be heard or seen.
So you will still need an original "The Settlers 2 Gold Edition" version to play Return To The Roots.

# How to build

```
git clone https://github.com/Return-To-The-Roots/s25client s25client
cd s25client
git submodules update --init
cd build
./cmake.sh --prefix=.
make
```
