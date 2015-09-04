#ifndef LOCAL_H_INCLUDED
#define LOCAL_H_INCLUDED

#ifdef _WIN32
///////////////////////////////////////////////////////////////////////////////
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

//
///////////////////////////////////////////////////////////////////////////////
#endif

///////////////////////////////////////////////////////////////////////////////
//	Other Configuration

//	set global prefix (normal /usr/local)
//#define PREFIX "/usr/local"
#cmakedefine PREFIX "${PREFIX}"

//	set binary directory (normal $(prefix)/bin)
//#define BINDIR PREFIX"/bin"
#cmakedefine BINDIR "${BINDIR}"

//	set data directory (normal $(datadir)/s25rttr)
//#define DATADIR PREFIX"/share/s25rttr"
#cmakedefine DATADIR "${DATADIR}"

//	set game directory (normal $(datadir)/s25rttr/S2)
//#define GAMEDIR DATADIR"/S2"

//	set driver directory (normal $(libdir)/driver)
//#define DRIVERDIR LIBDIR"/driver"
#cmakedefine DRIVERDIR "${DRIVERDIR}"

//	set settings directory (normal $HOME/.s25rttr)
//#define SETTINGSDIR "~/.s25rttr"

//
///////////////////////////////////////////////////////////////////////////////

#endif // !LOCAL_H_INCLUDED
