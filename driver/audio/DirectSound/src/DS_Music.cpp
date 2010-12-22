// $Id: DS_Music.cpp 6458 2010-05-31 11:38:51Z FloSoft $
//
// Copyright (c) 2005-2009 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Siedler II.5 RTTR.
//
// Siedler II.5 RTTR is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Siedler II.5 RTTR is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Siedler II.5 RTTR. If not, see <http://www.gnu.org/licenses/>.

///////////////////////////////////////////////////////////////////////////////
// Header
#include "DS_Music.h"

#include <cstring>
#include <cstdio>


/// Konstruktor von @p SoundSDL_Music.
DS_Music::DS_Music(const char * const music_file)
{
	std::strcpy(this->music_file,music_file);
}

/// Destruktor von @p SoundSDL_Music.
DS_Music::~DS_Music()
{
	// Datei wieder löschen
	unlink(music_file);
}