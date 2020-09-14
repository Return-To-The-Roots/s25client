#pragma once

/// Path where the source was when the application got compiled
#cmakedefine RTTR_SRCDIR "@RTTR_SRCDIR@"

/// Install prefix path
#cmakedefine RTTR_INSTALL_PREFIX "@CMAKE_INSTALL_PREFIX@"

///	set binary directory (normal ./bin)
#cmakedefine RTTR_BINDIR "@RTTR_BINDIR@"

///	set extra binary directory (normal ./libexec)
#cmakedefine RTTR_EXTRA_BINDIR "@RTTR_EXTRA_BINDIR@"

///	set data directory (normal ./share/s25rttr)
#cmakedefine RTTR_DATADIR "@RTTR_DATADIR@"

///	set game directory (normal RTTR_DATADIR/S2)
#cmakedefine RTTR_GAMEDIR "@RTTR_GAMEDIR@"

///	set lib directory (normal RTTR_DATADIR)
#cmakedefine RTTR_LIBDIR "@RTTR_LIBDIR@"

///	set driver directory (normal RTTR_LIBDIR/driver)
#cmakedefine RTTR_DRIVERDIR "@RTTR_DRIVERDIR@"
