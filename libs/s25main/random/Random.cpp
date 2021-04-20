// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "random/Random.h"
#include "s25util/Serializer.h"
#include <stdexcept>

template<class T_PRNG>
int calcRandValue(T_PRNG& rng, int maxExcl)
{
    // Special case: [0, 0) makes 0
    if(maxExcl == 0)
        return 0;
    return static_cast<int>(rng() % static_cast<unsigned>(maxExcl));
}

template<class T_PRNG>
Random<T_PRNG>::Random()
{
    Init(123456789);
}

template<class T_PRNG>
void Random<T_PRNG>::Init(const uint64_t& seed)
{
    ResetState(PRNG(static_cast<typename PRNG::result_type>(seed)));
}

template<class T_PRNG>
void Random<T_PRNG>::ResetState(const PRNG& newState)
{
    rng_ = newState;
    numInvocations_ = 0;
}

template<class T_PRNG>
int Random<T_PRNG>::Rand(const RandomContext& context, const int maxExcl)
{
    history_[numInvocations_ % history_.size()] = RandomEntry(numInvocations_, maxExcl, rng_, context);
    ++numInvocations_;

    return calcRandValue(rng_, maxExcl);
}

template<class T_PRNG>
unsigned Random<T_PRNG>::GetChecksum() const
{
    return CalcChecksum(rng_);
}

template<class T_PRNG>
unsigned Random<T_PRNG>::CalcChecksum(const PRNG& rng)
{
    // This is designed in a way that makes the checksum be equivalent
    // to the state of the OldLCG for compatibility with old versions
    Serializer ser;
    rng.serialize(ser);
    unsigned checksum = 0;
    while(ser.GetBytesLeft() >= sizeof(unsigned))
        checksum += ser.PopUnsignedInt();
    while(ser.GetBytesLeft())
        checksum += ser.PopUnsignedChar();
    return checksum;
}

template<class T_PRNG>
const typename Random<T_PRNG>::PRNG& Random<T_PRNG>::GetCurrentState() const
{
    return rng_;
}

template<class T_PRNG>
std::vector<typename Random<T_PRNG>::RandomEntry> Random<T_PRNG>::GetAsyncLog()
{
    std::vector<RandomEntry> ret;

    unsigned begin, end;
    if(numInvocations_ > history_.size())
    {
        // Ringbuffer filled -> Start from next entry (which is the one written longest time ago)
        // and go one full cycle (to the entry written last)
        begin = numInvocations_;
        end = numInvocations_ + history_.size();
    } else
    {
        // Ringbuffer not filled -> Start from 0 till number of entries
        begin = 0;
        end = numInvocations_;
    }

    ret.reserve(end - begin);
    for(unsigned i = begin; i < end; ++i)
        ret.push_back(history_[i % history_.size()]);

    return ret;
}

//////////////////////////////////////////////////////////////////////////

template<class T_PRNG>
void Random<T_PRNG>::RandomEntry::Serialize(Serializer& ser) const
{
    ser.PushUnsignedInt(counter);
    ser.PushSignedInt(maxExcl);
    // We save the type a) for double checking and b) for future extension
    ser.PushLongString(T_PRNG::getName());
    rngState.serialize(ser);
    ser.PushLongString(srcName);
    ser.PushUnsignedInt(srcLine);
    ser.PushUnsignedInt(objId);
}

template<class T_PRNG>
void Random<T_PRNG>::RandomEntry::Deserialize(Serializer& ser)
{
    counter = ser.PopUnsignedInt();
    maxExcl = ser.PopSignedInt();
    std::string name = ser.PopLongString();
    if(name != T_PRNG::getName())
        throw std::runtime_error("Wrong random number generator");
    rngState.deserialize(ser);
    srcName = ser.PopLongString();
    srcLine = ser.PopUnsignedInt();
    objId = ser.PopUnsignedInt();
}

template<class T_PRNG>
int Random<T_PRNG>::RandomEntry::GetValue() const
{
    PRNG tmpRng(rngState);
    return calcRandValue(tmpRng, maxExcl);
}

// Instantiate the Random class with the used PRNG
template class Random<UsedPRNG>;
