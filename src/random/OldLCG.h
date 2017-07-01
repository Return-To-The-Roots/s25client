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

#ifndef OldLCG_h__
#define OldLCG_h__

#include <boost/core/enable_if.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <iosfwd>
#include <boost/cstdint.hpp>

class Serializer;

/// Legacy linear congruent (like) generator
class OldLCG
{
public:
    typedef unsigned result_type;

    static result_type min() { return 0; }
    static result_type max() { return 32767; }

    OldLCG() { seed(); }
    explicit OldLCG(result_type initSeed) { seed(initSeed); }
    template<class T_SeedSeq>
    explicit OldLCG(T_SeedSeq& seedSeq, typename boost::disable_if<boost::is_integral<T_SeedSeq> >::type* dummy = 0) { seed(seedSeq); }

    void seed() { seed(0x1337); }
    void seed(unsigned newSeed) { state_ = newSeed; }
    template<class T_SeedSeq>
    void seed(T_SeedSeq& seedSeq, typename boost::disable_if<boost::is_integral<T_SeedSeq> >::type* dummy = 0);

    /// Return random value in [min, max]
    result_type operator()() { return (*this)(max()); }
    /// Return random value in [0, maxVal] for a small maxVal
    result_type operator()(result_type maxVal);

    void discard(uint64_t j);

    void Serialize(Serializer& ser) const;
    void Deserialize(Serializer& ser);
private:
    friend std::ostream& operator<<(std::ostream& os, const OldLCG& obj);
    friend std::istream& operator>>(std::istream& is, OldLCG& obj);
    friend bool operator==(const OldLCG& lhs, const OldLCG& rhs){ return lhs.state_ == rhs.state_; }
    friend bool operator!=(const OldLCG& lhs, const OldLCG& rhs){ return !(lhs == rhs); }

    unsigned state_;
};

template<class T_SeedSeq>
inline void OldLCG::seed(T_SeedSeq& seedSeq, typename boost::disable_if<boost::is_integral<T_SeedSeq> >::type*)
{
    unsigned seedVal;
    seedSeq.generate(&seedVal, &seedVal + 1);
    seed(seedVal);
}

inline OldLCG::result_type OldLCG::operator()(OldLCG::result_type maxVal)
{
    // Shift range from [0, maxVal) to [0, maxVal]
    maxVal++;
    state_ = (state_ * 997 + 1 + maxVal) & 32767;
    return (state_ * maxVal) / 32768;
}

#endif // OldLCG_h__
