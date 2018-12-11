// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef DefaultLCG_h__
#define DefaultLCG_h__

#include <boost/type_traits/is_integral.hpp>
#include <boost/utility/enable_if.hpp>
#include <iosfwd>

class Serializer;

/// Legacy linear congruent (like) generator
class DefaultLCG
{
public:
    typedef uint32_t result_type;

    static result_type min() { return 0; }
    static result_type max() { return 0xFFFF; }
    static const char* getName() { return "DefaultLCG"; }

    DefaultLCG() { seed(); }
    explicit DefaultLCG(result_type initSeed) { seed(initSeed); }
    template<class T_SeedSeq>
    explicit DefaultLCG(T_SeedSeq& seedSeq, typename boost::disable_if<boost::is_integral<T_SeedSeq> >::type* = 0)
    {
        seed(seedSeq);
    }

    void seed() { seed(0x1337); }
    void seed(unsigned newSeed) { state_ = newSeed; }
    template<class T_SeedSeq>
    void seed(T_SeedSeq& seedSeq, typename boost::disable_if<boost::is_integral<T_SeedSeq> >::type* = 0);

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
inline void DefaultLCG::seed(T_SeedSeq& seedSeq, typename boost::disable_if<boost::is_integral<T_SeedSeq> >::type*)
{
    unsigned seedVal;
    seedSeq.generate(&seedVal, &seedVal + 1);
    seed(seedVal);
}

inline DefaultLCG::result_type DefaultLCG::operator()()
{
    // Constants taken from 'Numerical Recipes'
    state_ = state_ * 1664525u + 1013904223u;
    // Upper bits have higher period
    return state_ >> 16;
}

#endif // DefaultLCG_h__
