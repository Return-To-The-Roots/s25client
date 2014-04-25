// $Id: strlwr.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef STRLWR_H_INCLUDED
#define STRLWR_H_INCLUDED

#pragma once

/*
FUNCTION
    <<strlwr>>---force string to lower case

INDEX
    strlwr

ANSI_SYNOPSIS
    #include <strlwr.h>
    char *strlwr(char *<[str]>);

TRAD_SYNOPSIS
    #include <string.h>
    char *strlwr(<[str]>)
    char *<[str]>;

DESCRIPTION
    <<strlwr>> converts each characters in the string at <[str]> to
    lower case.

RETURNS
    <<strlwr>> returns its argument, <[str]>.

PORTABILITY
<<strlwr>> is not widely portable.

<<strlwr>> requires no supporting OS subroutines.

QUICKREF
    strlwr
*/

#include <string.h>
#include <ctype.h>

inline char* strlwr(char* str)
{
    char* ret = str;

    while (*str != '\0')
    {
        if (isupper (*str))
            *str = tolower (*str);
        ++str;
    }

    return ret;
}

#endif // !STRLWR_H_INCLUDED
