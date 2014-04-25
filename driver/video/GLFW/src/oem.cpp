// $Id: oem.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "main.h"
#include "oem.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifndef _WIN32

///////////////////////////////////////////////////////////////////////////////
/** @name AnsiToOem
 *
 *  Wandelt einen String vom ANSI ins OEM Format um.
 *
 *  @author     FloSoft
 *
 *  @param[in]  from   konstanter Quellstring
 *  @param[out] to     Zielstring (ausreichend Speicher muss vorhanden sein)
 *  @param[in]  length Länge des Quellstrings, wenn 0 wird @p strlen(from) verwendet
 *
 *  @return            @p to wird zurückgeliefert
 */
char* AnsiToOem(const char* from, char* to, unsigned int length)
{
    /// Konvertiertabelle von ANSI nach OEM, beginnend bei char 128
    static unsigned char ansi2oem_tab[] =
    {
        /*0080:*/ 0x00, 0x00, 0x00, 0x9F, 0x00, 0x00, 0x00, 0xD8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /*0090:*/ 0x00, 0x60, 0x27, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /*00A0:*/ 0xFF, 0xAD, 0x9B, 0x9C, 0x0F, 0x9D, 0x7C, 0x15, 0x22, 0x63, 0xA6, 0xAE, 0xAA, 0x2D, 0x52, 0x00,
        /*00B0:*/ 0xF8, 0xF1, 0xFD, 0x33, 0x27, 0xE6, 0x14, 0xFA, 0x2C, 0x31, 0xA7, 0xAF, 0xAC, 0xAB, 0x00, 0xA8,
        /*00C0:*/ 0x41, 0x41, 0x41, 0x41, 0x8E, 0x8F, 0x92, 0x80, 0x45, 0x90, 0x45, 0x45, 0x49, 0x49, 0x49, 0x49,
        /*00D0:*/ 0x44, 0xA5, 0x4F, 0x4F, 0x4F, 0x4F, 0x99, 0x78, 0x4F, 0x55, 0x55, 0x55, 0x9A, 0x59, 0x00, 0xE1,
        /*00E0:*/ 0x85, 0xA0, 0x83, 0x61, 0x84, 0x86, 0x91, 0x87, 0x8A, 0x82, 0x88, 0x89, 0x8D, 0xA1, 0x8C, 0x8B,
        /*00F0:*/ 0x64, 0xA4, 0x95, 0xA2, 0x93, 0x6F, 0x94, 0xF6, 0x6F, 0x97, 0xA3, 0x96, 0x81, 0x79, 0x00, 0x98,
    };

    // sanity check
    if(to == NULL || from == NULL)
        return NULL;

    // wir haben keine Länge erhalten, also ermitteln
    if(length == 0)
        length = (unsigned int)strlen(from);

    // und string umwandeln
    for(unsigned int x = 0; x < length; x++)
    {
        if(from[x])
        {
            unsigned char C = (unsigned char)from[x];

            // ab char 128 nötig
            if(C > 128)
                to[x] = (char)ansi2oem_tab[C & 0x7F];
            else
                to[x] = from[x];
        }
    }
    return to;
}

///////////////////////////////////////////////////////////////////////////////
/** @name OemToAnsi
 *
 *  Wandelt einen String vom OEM ins ANSI Format um.
 *
 *  @author     FloSoft
 *
 *  @param[in]  from   konstanter Quellstring
 *  @param[out] to     Zielstring (ausreichend Speicher muss vorhanden sein)
 *  @param[in]  length Länge des Quellstrings, wenn 0 wird @p strlen(from) verwendet
 *
 *  @return            @p to wird zurückgeliefert
 */
char* OemToAnsi(const char* from, char* to, unsigned int length)
{
    /// Konvertiertabelle von OEM nach ANSI
    static unsigned char ansi2oem_tab[256] =
    {
        /*0000:*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        /*0010:*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        /*0020:*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        /*0030:*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        /*0040:*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        /*0050:*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        /*0060:*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        /*0070:*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        /*0080:*/ 0x00, 0x00, 0x00, 0x9F, 0x00, 0x00, 0x00, 0xD8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /*0090:*/ 0x00, 0x60, 0x27, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /*00A0:*/ 0xFF, 0xAD, 0x9B, 0x9C, 0x0F, 0x9D, 0x7C, 0x15, 0x22, 0x63, 0xA6, 0xAE, 0xAA, 0x2D, 0x52, 0x00,
        /*00B0:*/ 0xF8, 0xF1, 0xFD, 0x33, 0x27, 0xE6, 0x14, 0xFA, 0x2C, 0x31, 0xA7, 0xAF, 0xAC, 0xAB, 0x00, 0xA8,
        /*00C0:*/ 0x41, 0x41, 0x41, 0x41, 0x8E, 0x8F, 0x92, 0x80, 0x45, 0x90, 0x45, 0x45, 0x49, 0x49, 0x49, 0x49,
        /*00D0:*/ 0x44, 0xA5, 0x4F, 0x4F, 0x4F, 0x4F, 0x99, 0x78, 0x4F, 0x55, 0x55, 0x55, 0x9A, 0x59, 0x00, 0xE1,
        /*00E0:*/ 0x85, 0xA0, 0x83, 0x61, 0x84, 0x86, 0x91, 0x87, 0x8A, 0x82, 0x88, 0x89, 0x8D, 0xA1, 0x8C, 0x8B,
        /*00F0:*/ 0x64, 0xA4, 0x95, 0xA2, 0x93, 0x6F, 0x94, 0xF6, 0x6F, 0x97, 0xA3, 0x96, 0x81, 0x79, 0x00, 0x98,
    };

    // sanity check
    if(to == NULL || from == NULL)
        return NULL;

    // wir haben keine Länge erhalten, also ermitteln
    if(length == 0)
        length = (unsigned int)strlen(from);

    // und string umwandeln
    for(unsigned int x = 0; x < length; ++x)
    {
        if((unsigned char)from[x] > 128)
        {
            for(int i = 0; i < 256; ++i)
            {
                if((unsigned char)from[x] == ansi2oem_tab[i])
                {
                    to[x] = (char)i;
                    break;
                }
            }
        }
        else
            to[x] = from[x];
    }
    return to;
}

#endif // !_WIN32
