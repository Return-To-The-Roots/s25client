// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>
#include <iosfwd>
#include <type_traits>

class Serializer;

/// Legacy linear congruent (like) generator
class DefaultLCG
{
public:
    using result_type = uint32_t;

    static constexpr result_type min() { return 0; }
    static constexpr result_type max() { return 0xFFFF; }
    static constexpr const char* getName() { return "DefaultLCG"; }

    DefaultLCG() { seed(); }
    explicit DefaultLCG(result_type initSeed) { seed(initSeed); }
    template<class T_SeedSeq>
    explicit DefaultLCG(T_SeedSeq& seedSeq, std::enable_if_t<!std::is_integral_v<T_SeedSeq>>* = nullptr)
    {
        seed(seedSeq);
    }

    void seed() { seed(0x1337); }
    void seed(unsigned newSeed) { state_ = newSeed; }
    template<class T_SeedSeq>
    void seed(T_SeedSeq& seedSeq, std::enable_if_t<!std::is_integral_v<T_SeedSeq>>* = nullptr);

    /// Return random value in [min, max]
    result_type operator()();

    void discard(uint64_t j);

    void serialize(Serializer& ser) const;
    void deserialize(Serializer& ser);

private:
    friend std::ostream& operator<<(std::ostream& os, const DefaultLCG& obj);
    friend std::istream& operator>>(std::istream& is, DefaultLCG& obj);
    friend bool operator==(const DefaultLCG& lhs, const DefaultLCG& rhs) { return lhs.state_ == rhs.state_; }
    friend bool operator!=(const DefaultLCG& lhs, const DefaultLCG& rhs) { return !(lhs == rhs); }

    unsigned state_;
};

template<class T_SeedSeq>
inline void DefaultLCG::seed(T_SeedSeq& seedSeq, std::enable_if_t<!std::is_integral_v<T_SeedSeq>>*)
{
    unsigned seedVal;
    seedSeq.generate(&seedVal, (&seedVal) + 1);
    seed(seedVal);
}

inline DefaultLCG::result_type DefaultLCG::operator()()
{
    // Constants taken from 'Numerical Recipes'
    state_ = state_ * 1664525u + 1013904223u;
    // Upper bits have higher period
    return state_ >> 16;
}
