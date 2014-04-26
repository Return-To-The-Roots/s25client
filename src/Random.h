// $Id: Random.h 9363 2014-04-26 15:00:08Z FloSoft $
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
#ifndef RANDOM_H_INCLUDED
#define RANDOM_H_INCLUDED

#pragma once

#include "Singleton.h"
#include <list>

struct RandomEntry
{
    unsigned counter;
    int max;
    int value;
    const char* src_name;
    unsigned src_line;
    unsigned obj_id;

    RandomEntry(unsigned counter, int max, int value, const char* src_name, unsigned int src_line, unsigned obj_id) : counter(counter), max(max), value(value), src_name(src_name), src_line(src_line), obj_id(obj_id) {};
    RandomEntry() : counter(0), max(0), value(0), src_name(NULL), src_line(0), obj_id(0) {};
};

class Random : public Singleton<Random>
{
        unsigned counter;
//  std::list<RandomEntry> async_log;
        RandomEntry async_log[1024];

    public:

        /// Konstruktor von @p Random.
        Random();
        /// Initialisiert den Zufallszahlengenerator.
        void Init(const unsigned int init);
        /// Erzeugt eine Zufallszahl.
        int Rand(const char* const src_name, const unsigned src_line, const unsigned obj_id, const int max);

        template <typename T>
        void Shuffle(T* const elements, unsigned int length, unsigned int repeat = 3)
        {
            for(unsigned int i = 0; i < repeat; ++i)
            {
                for(unsigned int j = 0; j < length; j++)
                {
                    unsigned int to = Rand(__FILE__, __LINE__, 0, length);

                    T temp = elements[i];
                    elements[i] = elements[to];
                    elements[to] = temp;
                }
            }
        }

        /// Gibt aktuelle Zufallszahl zurück
        int GetCurrentRandomValue() const { return zahl; }
        void ReplaySet(const unsigned int checksum) { zahl = checksum; }

        std::list<RandomEntry>* GetAsyncLog();

        /// Speichere Log
        void SaveLog(const char* const filename);

    private:
        int zahl; ///< Die aktuelle Zufallszahl.
};

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#define RANDOM Random::inst()

#endif // !RANDOM_H_INCLUDED
