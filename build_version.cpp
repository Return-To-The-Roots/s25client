// $Id: build_version.cpp 7245 2011-06-07 15:19:11Z FloSoft $
//
// Copyright (c) 2005 - 2010 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

///////////////////////////////////////////////////////////////////////////////
// Header
#include "stdafx.h"

#include <build_version.h>
#include <cstring>

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
	#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
#endif

const char *GetWindowTitle()
{
	static char title[256];
	std::memset(title, 0, 256);
	std::strncpy(title, WINDOW_TITLE, 256);
	return title;
}

const char *GetWindowVersion()
{
	static char version[256];
	std::memset(version, 0, 256);
	std::strncpy(version, WINDOW_VERSION, 256);
	return version;
}

const char *GetWindowRevision()
{
	static char revision[256];
	std::memset(revision, 0, 256);
	std::strncpy(revision, WINDOW_REVISION, 256);
	return revision;
}
