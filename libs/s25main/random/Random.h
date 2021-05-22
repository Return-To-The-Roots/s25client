// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "RTTR_Assert.h"
#include "random/XorShift.h"
#include "s25util/Singleton.h"
#include <array>
#include <cstddef>
#include <limits>
#include <string>
#include <utility>
#include <vector>

class Serializer;
/// Struct similar to std::source_location but includes the objId
struct RandomContext
{
    const char* srcName;
    unsigned srcLine;
    unsigned objId;
};

/// Random class for the random values in the game
/// Guarantees reproducible sequences given same seeds/states
/// Allows getting/restoring the state and provides a log of the last invocations and results
/// T_PRNG must be a model of the Pseudo-Random Number Generator according to boost:
///        http://www.boost.org/doc/libs/1_61_0/doc/html/boost_random/reference.html#boost_random.reference.concepts.pseudo_random_number_generator
/// Additionally it must implement Serialize and Deserialize functions and provide a static GetName function
template<class T_PRNG>
class Random : public Singleton<Random<T_PRNG>>
{
public:
    /// The used random number generator type
    using PRNG = T_PRNG;

    /// Class for storing the invocation of the rng
    struct RandomEntry
    {
        unsigned counter;
        int maxExcl;
        PRNG rngState;
        std::string srcName;
        unsigned srcLine;
        unsigned objId;

        RandomEntry() : counter(0), maxExcl(0), srcLine(0), objId(0){};
        RandomEntry(unsigned counter, int maxExcl, const PRNG& rngState, const RandomContext& ctx)
            : counter(counter), maxExcl(maxExcl), rngState(rngState), srcName(ctx.srcName), srcLine(ctx.srcLine),
              objId(ctx.objId){};

        void Serialize(Serializer& ser) const;
        void Deserialize(Serializer& ser);

        int GetValue() const;
    };

    Random();
    /// Initialize the rng with a given seed
    void Init(const uint64_t& seed);
    /// Reset the Random class to start from a given state
    void ResetState(const PRNG& newState);
    /// Return a random number in the range [0, maxExcl)
    int Rand(const RandomContext& context, int maxExcl);

    /// Get a checksum of the RNG
    unsigned GetChecksum() const;
    static unsigned CalcChecksum(const PRNG& rng);

    /// Get current rng state
    const PRNG& GetCurrentState() const;

    std::vector<RandomEntry> GetAsyncLog();

private:
    PRNG rng_; /// the PRNG
    /// Number of invocations to the PRNG
    unsigned numInvocations_;
    /// History
    std::array<RandomEntry, 1024> history_; //-V730_NOINIT
};

/// The actual PRNG used for the ingame RNG
using UsedPRNG = XorShift;
using UsedRandom = Random<UsedPRNG>;
using RandomEntry = UsedRandom::RandomEntry;

///////////////////////////////////////////////////////////////////////////////
// Macros / Defines
#define RANDOM UsedRandom::inst()
#define RANDOM_CONTEXT() \
    RandomContext { __FILE__, __LINE__, GetObjId() }
#define RANDOM_CONTEXT2(objId) \
    RandomContext { __FILE__, __LINE__, objId }
/// Shortcut to get a new random value in range [0, maxVal) for a given object id
/// Note: maxVal has to be small (at least <= 32768)
#define RANDOM_RAND(maxValExcl) RANDOM.Rand(RANDOM_CONTEXT(), maxValExcl)
/// Return a random element from the container. Must not be empty
#define RANDOM_ELEMENT(container) detail::randomElement(container, RANDOM_CONTEXT())
/// Return a random enumerator of the given type. Requires the <helpers/MaxEnumValue.h> include
#define RANDOM_ENUM(EnumType) static_cast<EnumType>(RANDOM_RAND(helpers::NumEnumValues_v<EnumType>))
/// Shuffle the given container
#define RANDOM_SHUFFLE(container) detail::shuffleContainer(container, RANDOM_CONTEXT())
#define RANDOM_SHUFFLE2(container, objId) detail::shuffleContainer(container, RANDOM_CONTEXT2(objId))

namespace detail {
template<typename T>
auto randomElement(const T& container, const RandomContext& ctx)
{
    RTTR_Assert(!container.empty());
    return *(container.begin() + RANDOM.Rand(ctx, container.size()));
}
template<class T>
static void shuffleContainer(T& container, const RandomContext& ctx)
{
    if(container.empty())
        return;
    for(int i = container.size() - 1; i > 0; --i)
    {
        using std::swap;
        swap(container[i], container[RANDOM.Rand(ctx, i + 1)]);
    }
}
} // namespace detail
