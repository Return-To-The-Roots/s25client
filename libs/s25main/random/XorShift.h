// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <cstdint>
#include <iosfwd>
#include <limits>
#include <type_traits>

class Serializer;

/// XorShift64* RNG according to http://vigna.di.unimi.it/ftp/papers/xorshift.pdf
class XorShift
{
public:
    using result_type = uint64_t;

    static constexpr result_type min() { return 1; }
    static constexpr result_type max() { return std::numeric_limits<uint64_t>::max(); }
    static constexpr const char* getName() { return "XorShift"; }

    XorShift() { seed(); }
    explicit XorShift(uint64_t initSeed) { seed(initSeed); }
    template<class T_SeedSeq>
    explicit XorShift(T_SeedSeq& seedSeq, std::enable_if_t<!std::is_integral<T_SeedSeq>::value>* = nullptr)
    {
        seed(seedSeq);
    }

    void seed() { seed(0x1337); }
    void seed(uint64_t newSeed);
    template<class T_SeedSeq>
    void seed(T_SeedSeq& seedSeq, std::enable_if_t<!std::is_integral<T_SeedSeq>::value>* = nullptr);

    /// Return random value in [min, max]
    result_type operator()();

    void discard(uint64_t j);

    void serialize(Serializer& ser) const;
    void deserialize(Serializer& ser);

private:
    friend std::ostream& operator<<(std::ostream& os, const XorShift& obj);
    friend std::istream& operator>>(std::istream& is, XorShift& obj);
    friend bool operator==(const XorShift& lhs, const XorShift& rhs) { return lhs.state_ == rhs.state_; }
    friend bool operator!=(const XorShift& lhs, const XorShift& rhs) { return !(lhs == rhs); }

    uint64_t state_; //-V730_NOINIT
};

template<class T_SeedSeq>
inline void XorShift::seed(T_SeedSeq& seedSeq, std::enable_if_t<!std::is_integral<T_SeedSeq>::value>*)
{
    std::array<uint32_t, 2> seeds;
    seedSeq.generate(seeds.begin(), seeds.end());
    // Interpret 2 32 bit values as one 64 bit value
    seed((static_cast<uint64_t>(seeds[0]) << 32) | seeds[1]);
}

inline XorShift::result_type XorShift::operator()()
{
    state_ ^= state_ >> 12; // a
    state_ ^= state_ << 25; // b
    state_ ^= state_ >> 27; // c
    return state_ * UINT64_C(2685821657736338717);
}
