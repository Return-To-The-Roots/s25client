// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "random/Random.h"
#include "s25util/Serializer.h"
#include <stdexcept>

template<class T_PRNG>
int calcRandValue(T_PRNG& rng, int max)
{
    // Special case: [0, 0) makes 0
    if(max == 0)
        return 0;
    return static_cast<int>(rng() % static_cast<unsigned>(max));
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
int Random<T_PRNG>::Rand(const char* const src_name, const unsigned src_line, const unsigned obj_id, const int max)
{
    history_[numInvocations_ % history_.size()] = RandomEntry(numInvocations_, max, rng_, src_name, src_line, obj_id);
    ++numInvocations_;

    return calcRandValue(rng_, max);
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
    ser.PushSignedInt(max);
    // We save the type a) for double checking and b) for future extension
    ser.PushLongString(T_PRNG::getName());
    rngState.serialize(ser);
    ser.PushLongString(src_name);
    ser.PushUnsignedInt(src_line);
    ser.PushUnsignedInt(obj_id);
}

template<class T_PRNG>
void Random<T_PRNG>::RandomEntry::Deserialize(Serializer& ser)
{
    counter = ser.PopUnsignedInt();
    max = ser.PopSignedInt();
    std::string name = ser.PopLongString();
    if(name != T_PRNG::getName())
        throw std::runtime_error("Wrong random number generator");
    rngState.deserialize(ser);
    src_name = ser.PopLongString();
    src_line = ser.PopUnsignedInt();
    obj_id = ser.PopUnsignedInt();
}

template<class T_PRNG>
int Random<T_PRNG>::RandomEntry::GetValue() const
{
    PRNG tmpRng(rngState);
    return calcRandValue(tmpRng, max);
}

// Instantiate the Random class with the used PRNG
template class Random<UsedPRNG>;
