// $Id: Random.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
// Header
#include "main.h"
#include "Random.h"

#include "GameClient.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p Random.
 *
 *  @author FloSoft
 */
Random::Random(void)
{
    /*log = fopen("async_log.txt","w");*/
    Init(123456789);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Initialisiert den Zufallszahlengenerator.
 *
 *  @param[in] init Zahl mit der der Zufallsgenerator initialisiert wird.
 *
 *  @author OLiver
 */
void Random::Init(const unsigned int init)
{
    zahl = init;
    counter = 0;
//  async_log.clear();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Erzeugt eine Zufallszahl.
 *
 *  @param[in] max @p max-1 ist die maximale Zufallszahl welche geliefert werden soll.
 *
 *  @return liefert eine Zufallszahl.
 *
 *  @author OLiver
 */
int Random::Rand(const char* const src_name, const unsigned src_line, const unsigned obj_id, const int max)
{
    zahl = ( (zahl * 997) + 1 + max) & 32767;

    async_log[counter % 1024] = RandomEntry(counter, max, zahl, src_name, src_line, obj_id);

    /*  async_log.push_back();
        if(async_log.size() > 10000)
            async_log.pop_front();*/

    ++counter;
    return ( (zahl * max) / 32768);
}

std::list<RandomEntry> *Random::GetAsyncLog()
{
    std::list<RandomEntry> *ret = new std::list<RandomEntry>;

    unsigned int max = (counter > 1024 ? 1024 : counter);
    for (unsigned int i = 0; i < max; ++i)
    {
        ret->push_back(async_log[(i + counter) % 1024]);
    }

    return(ret);
}

void Random::SaveLog(const char* const filename)
{
    FILE* file = fopen(filename, "w");

    unsigned int max = (counter > 1024 ? 1024 : counter);
    for (unsigned int i = 0; i < max; ++i)
    {
        RandomEntry* it = &(async_log[(i + counter) % 1024]);
        fprintf(file, "%u:R(%d)=%d,z=%d | %s Z: %u|id=%u\n", it->counter, it->max, (it->value * it->max) / 32768, it->value, it->src_name, it->src_line, it->obj_id);
    }

    fclose(file);
}
