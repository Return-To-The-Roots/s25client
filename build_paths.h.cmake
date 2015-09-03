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

#endif // !LOCAL_H_INCLUDED
