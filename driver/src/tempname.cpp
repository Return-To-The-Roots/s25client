// $Id: tempname.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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

///////////////////////////////////////////////////////////////////////////////
// Systemheader
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

///////////////////////////////////////////////////////////////////////////////
// Header
#include "tempname.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/** @name tempname
 *
 *  liefert den Namen und Pfad einer temporären Datei.
 *
 *  @author     FloSoft
 *
 *  @param[in,out] name   Pfad/Name der Datei
 *  @param[in]     length Größe des Speicherblocks von @p name
 *
 *  @return               bei Erfolg wird true geliefert, bei Fehler false
 */
bool tempname(char* name, unsigned int length)
{
    if(!name)
        return false;

    memset(name, 0, length);

    const char* tempdir = NULL;

    // Ort des Temporären Verzeichnisses holen
    tempdir = getenv("TMPDIR");
    if (tempdir == NULL || *tempdir == '\0')
        tempdir = getenv("TMP");
    if (tempdir == NULL || *tempdir == '\0')
        tempdir = getenv("TEMP");
#ifndef _WIN32
    if (tempdir == NULL)
        tempdir = "/tmp";
#endif
    if (tempdir == NULL)
        return false;

    unsigned int dirlen = (unsigned int)strlen(tempdir);

    while(dirlen > 0 && tempdir[dirlen - 1] == '/')
        dirlen--;

    srand( (unsigned int)time(NULL) );

    char temp[512];
    FILE* test = NULL;

    // nach einer unbenutzten Datei suchen
    do
    {
        if(test)
            fclose(test);

        memset(temp, 0, 512);

        for(unsigned int i = 0; i < 10; i++)
            sprintf(temp, "%s%c", temp, (rand() % 25) + 65 );
#if defined _WIN32
#ifndef __CYGWIN__
#define snprintf _snprintf
#endif
        snprintf(name, length, "%s\\tmp.%s", tempdir, temp);
#else
        snprintf(name, length, "%s/tmp.%s", tempdir, temp);
#endif // !_WIN32

        test = fopen(name, "r");
    }
    while ( test != NULL);

    return true;
}
