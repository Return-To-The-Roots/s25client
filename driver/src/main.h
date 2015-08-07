// $Id: main.h 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

#pragma once

///////////////////////////////////////////////////////////////////////////////
// System-Header
#ifdef _WIN32
#   define _CRTDBG_MAP_ALLOC
#   include <windows.h>
#   include <io.h>
#else
#   include <unistd.h>
#   include <limits.h>
#endif // !_WIN32

#if defined _WIN32 && defined _DEBUG
#   include <crtdbg.h>
#endif // _WIN32 && _DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#endif // !MAIN_H_INCLUDED
