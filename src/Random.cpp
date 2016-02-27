// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#include "defines.h" // IWYU pragma: keep
#include "Random.h"
#include <cstdio>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

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
    rngState_ = init;
    counter = 0;
}

int Random::GetValueFromState(const int rngState, const int maxVal)
{
    return (rngState * maxVal) / 32768;
}

int Random::GetNextState(const int rngState, const int maxVal)
{
    return (rngState * 997 + 1 + maxVal) & 32767;
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
    rngState_ = GetNextState(rngState_, max);

    async_log[counter % async_log.size()] = RandomEntry(counter, max, rngState_, src_name, src_line, obj_id);
    ++counter;

    return GetValueFromState(rngState_, max);
}

std::vector<RandomEntry> Random::GetAsyncLog()
{
    std::vector<RandomEntry> ret;

    unsigned int max = (counter > async_log.size() ? async_log.size() : counter);
    for (unsigned int i = 0; i < max; ++i)
    {
        ret.push_back(async_log[(i + counter) % async_log.size()]);
    }

    return ret;
}

void Random::SaveLog(const std::string& filename)
{
    FILE* file = fopen(filename.c_str(), "w");

    unsigned int max = (counter > async_log.size() ? async_log.size() : counter);
    for (unsigned int i = 0; i < max; ++i)
    {
        RandomEntry& it = async_log[(i + counter) % async_log.size()];
        fprintf(file, "%u:R(%d)=%d,z=%d | %s#%u|id=%u\n", it.counter, it.max, GetValueFromState(it.rngState, it.max), it.rngState, it.src_name.c_str(), it.src_line, it.obj_id);
    }

    fclose(file);
}
