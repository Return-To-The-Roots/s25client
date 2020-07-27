# Info for package creators

The following is mostly applicable for package maintainers.

In short you might want to set the following CMake variables:

- `CMAKE_BUILD_TYPE=Release`
- `RTTR_VERSION=<x.y.z>` (as appropriate)
- `RTTR_USE_SYSTEM_LIBS=ON`
- `FETCHCONTENT_FULLY_DISCONNECTED=ON`
- `BUILD_TESTING=OFF`

Some files suitable for releases can be found in the `tools/release` folder.
Check e.g. for `tools/release/debian/s25rttr.png` for an icon.

## Build type

Option: `CMAKE_BUILD_TYPE`
Optimized ("release") builds are what you want to ship to users.
Alternatively use `RelWithDebInfo` to include debug info at the expense of a bit less optimization.

## Version

Option: `RTTR_VERSION`
RttR has 2 version parts: The version and revision.
The latter is the git commit hash for unique identification and determined from the git tree or a file `revision.txt` in the root folder.
The version is the human readable identification and defaults to the build date in `YYYYMMDD` format.

## System libs

Options: `RTTR_USE_SYSTEM_LIBS`, `RTTR_USE_SYSTEM_<lib>`
RttR uses some libraries which may be installed system-wide or from other sources.
To ease development it downloads those at configure time via CMakes [FetchContent](https://cmake.org/cmake/help/v3.14/module/FetchContent.html) module.
You can force the configure run to use an already installed library by setting the corresponding `RTTR_USE_SYSTEM_<lib>` variable to `ON`.
`RTTR_USE_SYSTEM_LIBS` is used to initialize all `RTTR_USE_SYSTEM_<lib>` if they are not explicitely overwritten.
So you may set that instead, but that has to be done on the first `cmake` run or the other variables will already be initialized.

## FetchContent

Option: `FETCHCONTENT_FULLY_DISCONNECTED`
The CMake [FetchContent](https://cmake.org/cmake/help/v3.14/module/FetchContent.html) module allows to download dependencies at configure time.
To avoid this the above variable can be set to `ON`.

As an alternative to using a system installed library you can checkout the library manually and set `FETCHCONTENT_SOURCE_DIR_<lib>` to the path of that checkout.
**Warning**: You are responsible for making sure the version matches the required one, so check the `FetchContent_Declare` commands in the source.

The name for `<lib>` matches the (properly cased) one used for `RTTR_USE_SYSTEM_<lib>` where the uppercase version uses `_` to separate camel cased words.
The only exception are library names like `LibFoo` which translate to `LIBFOO`.

## Tests

Option: `BUILD_TESTING`
By default the tests are build alongside Rttr.
You can disable this by setting the above (standard) option to `OFF` which e.g. avoids the dependency on Boost.Test.
