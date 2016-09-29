#ifndef BUILD_PATHS_H_INCLUDED
#define BUILD_PATHS_H_INCLUDED

#ifdef _WIN32
//	Windows Dependant-Configuration

//	disable Memory Leak Detection (comment it out to enable it)
#	define NOCRTDBG

//	disable catching of exceptions (comment it out to enable it)
#	define NOHWETRANS

#	ifdef _DEBUG
//		You can define paths here, but you should only use
//		those definitions in DEBUG-configuration.
#	endif // _DEBUG

//	if you don't define anything here, the current working
//	directory will be used to search the files and paths
//	(the s25rttr-files must be in the SETTLERS II installation folder)

#endif

///////////////////////////////////////////////////////////////////////////////

//	set global prefix (normal /usr/local)
#cmakedefine RTTR_PREFIX "@RTTR_PREFIX@"

//	set binary directory (normal RTTR_PREFIX/bin)
#cmakedefine RTTR_BINDIR "@RTTR_BINDIR@"

//	set data directory (normal RTTR_PREFIX/share/s25rttr)
#cmakedefine RTTR_DATADIR "@RTTR_DATADIR@"

//	set game directory (normal RTTR_DATADIR/S2)
#cmakedefine RTTR_GAMEDIR "@RTTR_GAMEDIR@"

//	set lib directory (normal RTTR_DATADIR)
#cmakedefine RTTR_LIBDIR "$@RTTR_LIBDIR@"

//	set driver directory (normal RTTR_LIBDIR/driver)
#cmakedefine RTTR_DRIVERDIR "@RTTR_DRIVERDIR@"

#endif // !BUILD_PATHS_H_INCLUDED
