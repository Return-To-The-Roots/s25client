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

#pragma once

#ifndef RANDOM_H_INCLUDED
#define RANDOM_H_INCLUDED

#include "Singleton.h"
#include <boost/array.hpp>
#include <vector>
#include <string>
#include <limits>

struct RandomEntry
{
    unsigned counter;
    int max;
    int rngState;
    std::string src_name;
    unsigned src_line;
    unsigned obj_id;

    inline unsigned GetValue() const;

    RandomEntry(unsigned counter, int max, int rngState, const std::string& src_name, unsigned int src_line, unsigned obj_id) : counter(counter), max(max), rngState(rngState), src_name(src_name), src_line(src_line), obj_id(obj_id) {};
    RandomEntry() : counter(0), max(0), rngState(0), src_line(0), obj_id(0) {};
};

class Random : public Singleton<Random>
{
        unsigned counter;
        boost::array<RandomEntry, 1024> async_log; //-V730_NOINIT

    public:

        Random();
        /// Initialisiert den Zufallszahlengenerator.
        void Init(const unsigned int init);
        /// Erzeugt eine Zufallszahl.
        int Rand(const char* const src_name, const unsigned src_line, const unsigned obj_id, const int max);

        /// Gibt aktuelle Zufallszahl zurück
        int GetCurrentRandomValue() const { return rngState_; }

        std::vector<RandomEntry> GetAsyncLog();

        /// Speichere Log
        void SaveLog(const std::string& filename);

        static int GetValueFromState(const int rngState, const int maxVal);

    private:
        int rngState_; /// Die aktuelle Zufallszahl.
        static inline int GetNextState(const int rngState, const int maxVal);
};

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#define RANDOM Random::inst()

/// functor using RANDOM.Rand(...) e.g. for std::random_shuffle
struct RandomFunctor
{
    const char* file_;
    unsigned line_;
    RandomFunctor(const char* file, unsigned line): file_(file), line_(line){}

    ptrdiff_t operator()(ptrdiff_t max) const
    {
        RTTR_Assert(max < std::numeric_limits<int>::max());
        return RANDOM.Rand(file_, line_, 0, static_cast<int>(max));
    }
};

/// Shortcut for creating an instance of RandomFunctor
#define RANDOM_FUNCTOR(varName) RandomFunctor varName(__FILE__, __LINE__)

unsigned RandomEntry::GetValue() const {
    return Random::GetValueFromState(rngState, max);
}

#endif // !RANDOM_H_INCLUDED
