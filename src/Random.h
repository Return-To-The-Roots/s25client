// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "random/OldLCG.h"
#include "libutil/Singleton.h"
#include <boost/array.hpp>
#include <iosfwd>
#include <limits>
#include <stdint.h>
#include <string>
#include <vector>

class Serializer;

/// Random class for the random values in the game
/// Guarantees reproducible sequences given same seeds/states
/// Allows getting/restoring the state and provides a log of the last invocations and results
/// T_PRNG must be a model of the Pseudo-Random Number Generator according to boost:
///        http://www.boost.org/doc/libs/1_61_0/doc/html/boost_random/reference.html#boost_random.reference.concepts.pseudo_random_number_generator
/// Additionally it must implement Serialize and Deserialize functions
/// and operator()(unsigned max) to return a random value in the range [0, max)
template<class T_PRNG>
class Random : public Singleton<Random<T_PRNG> >
{
public:
    /// The used random number generator type
    typedef T_PRNG PRNG;

    /// Class for storing the invocation of the rng
    struct RandomEntry
    {
        unsigned counter;
        int max;
        PRNG rngState;
        std::string src_name;
        unsigned src_line;
        unsigned obj_id;

        RandomEntry() : counter(0), max(0), src_line(0), obj_id(0){};
        RandomEntry(unsigned counter, int max, const PRNG& rngState, const std::string& src_name, unsigned src_line, unsigned obj_id)
            : counter(counter), max(max), rngState(rngState), src_name(src_name), src_line(src_line), obj_id(obj_id){};

        friend std::ostream& operator<<(std::ostream& os, const RandomEntry& entry) { return entry.print(os); }
        std::ostream& print(std::ostream& os) const;

        void Serialize(Serializer& ser) const;
        void Deserialize(Serializer& ser);

        int GetValue() const;
    };

    Random();
    /// Initialize the rng with a given seed
    void Init(const uint64_t& seed);
    /// Reset the Random class to start from a given state
    void ResetState(const PRNG& newState);
    /// Return a random number in the range [0, max)
    int Rand(const char* const src_name, const unsigned src_line, const unsigned obj_id, const int max);

    /// Get a checksum of the RNG
    unsigned GetChecksum() const;
    static unsigned CalcChecksum(const PRNG& rng);

    /// Get current rng state
    const PRNG& GetCurrentState() const;

    std::vector<RandomEntry> GetAsyncLog();

    /// Save the log to a file
    void SaveLog(const std::string& filename);

private:
    PRNG rng_; /// the PRNG
    /// Number of invocations to the PRNG
    unsigned numInvocations_;
    /// History
    boost::array<RandomEntry, 1024> history_; //-V730_NOINIT
};

/// The actual PRNG used for the ingame RNG
typedef OldLCG UsedPRNG;
typedef Random<UsedPRNG> UsedRandom;
typedef UsedRandom::RandomEntry RandomEntry;

///////////////////////////////////////////////////////////////////////////////
// Macros / Defines
#define RANDOM UsedRandom::inst()
/// Shortcut to get a new random value in range [0, maxVal) for a given object id
/// Note: maxVal has to be small (at least <= 32768)
#define RANDOM_RAND(objId, maxVal) RANDOM.Rand(__FILE__, __LINE__, objId, maxVal)

/// functor using RANDOM.Rand(...) e.g. for std::random_shuffle
struct RandomFunctor
{
    const char* file_;
    unsigned line_;
    RandomFunctor(const char* file, unsigned line) : file_(file), line_(line) {}

    ptrdiff_t operator()(ptrdiff_t max) const
    {
        RTTR_Assert(max < std::numeric_limits<int>::max());
        return RANDOM.Rand(file_, line_, 0, static_cast<int>(max));
    }
};

/// Shortcut for creating an instance of RandomFunctor
#define RANDOM_FUNCTOR(varName) RandomFunctor varName(__FILE__, __LINE__)

#endif // !RANDOM_H_INCLUDED
