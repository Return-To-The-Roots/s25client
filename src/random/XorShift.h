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

#ifndef XorShift_h__
#define XorShift_h__

#include <boost/array.hpp>
#include <boost/cstdint.hpp>
#include <boost/limits.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/utility/enable_if.hpp>
#include <iosfwd>

class Serializer;

/// XorShift64* RNG according to http://vigna.di.unimi.it/ftp/papers/xorshift.pdf
class XorShift
{
public:
    typedef uint64_t result_type;

    static result_type min() { return 1; }
    static result_type max() { return std::numeric_limits<uint64_t>::max(); }

    XorShift() { seed(); }
    explicit XorShift(uint64_t initSeed) { seed(initSeed); }
    template<class T_SeedSeq>
    explicit XorShift(T_SeedSeq& seedSeq, typename boost::disable_if<boost::is_integral<T_SeedSeq> >::type* dummy = 0)
    {
        seed(seedSeq);
    }

    void seed() { seed(0x1337); }
    void seed(uint64_t newSeed);
    template<class T_SeedSeq>
    void seed(T_SeedSeq& seedSeq, typename boost::disable_if<boost::is_integral<T_SeedSeq> >::type* dummy = 0);

    /// Return random value in [min, max]
    result_type operator()();
    /// Return random value in [0, maxVal] for a small maxVal
    unsigned operator()(unsigned maxVal);

    void discard(uint64_t j);

    void Serialize(Serializer& ser) const;
    void Deserialize(Serializer& ser);

private:
    friend std::ostream& operator<<(std::ostream& os, const XorShift& obj);
    friend std::istream& operator>>(std::istream& is, XorShift& obj);
    friend bool operator==(const XorShift& lhs, const XorShift& rhs) { return lhs.state_ == rhs.state_; }
    friend bool operator!=(const XorShift& lhs, const XorShift& rhs) { return !(lhs == rhs); }

    uint64_t state_;
};

template<class T_SeedSeq>
inline void XorShift::seed(T_SeedSeq& seedSeq, typename boost::disable_if<boost::is_integral<T_SeedSeq> >::type*)
{
    boost::array<uint32_t, 2> seeds;
    seedSeq.generate(seeds.begin(), seeds.end());
    // Interpret 2 32 bit values as one 64 bit value
    seed(*reinterpret_cast<uint64_t*>(seeds.data()));
}

inline XorShift::result_type XorShift::operator()()
{
    state_ ^= state_ >> 12; // a
    state_ ^= state_ << 25; // b
    state_ ^= state_ >> 27; // c
    return state_ * UINT64_C(2685821657736338717);
}

inline unsigned XorShift::operator()(unsigned maxVal)
{
    uint64_t value = (*this)() - 1;
    return static_cast<unsigned>(value % (maxVal + 1));
}

#endif // XorShift_h__
