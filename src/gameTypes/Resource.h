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

#ifndef Resource_h__
#define Resource_h__

/// Holds a resource and its value.
/// It makes sure that Type == Nothing => Amount == 0, but not vice versa!
/// Also: A value of 0 means Nothing|0
class Resource
{
    uint8_t value;

public:
    enum Type
    {
        Nothing,
        Iron,
        Gold,
        Coal,
        Granite,
        Water,
        Fish
    };
    BOOST_STATIC_CONSTEXPR int TypeCount = Fish + 1;

    Resource(Type type, uint8_t amount);
    explicit Resource(uint8_t value = 0);
    //explicit operator uint8_t() const { return value; }
    //explicit operator uint32_t() const { return value; }
    uint8_t getValue() const { return value; }
    Type getType() const { return Type(value >> 4); }
    uint8_t getAmount() const { return value & 0x0F; }
    void setType(Type newType);
    void setAmount(uint8_t newAmount);
    bool has(Type type) const { return getAmount() > 0u && getType() == type; }
    bool operator==(Resource rhs) const { return value == rhs.value; }
    bool operator!=(Resource rhs) const { return !(*this == rhs); }
};

inline Resource::Resource(Type type, uint8_t amount)
{
    if(type == Nothing)
        value = 0;
    else
        value = (static_cast<uint8_t>(type) << 4) | (amount & 0x0F);
}
inline Resource::Resource(uint8_t value) : value(value)
{
    if(getType() == Nothing || getType() >= TypeCount)
        value = 0;
}
inline void Resource::setType(Type newType)
{
    if(newType == Nothing)
        value = 0;
    else
        value = (static_cast<uint8_t>(newType) << 4) | getAmount();
}
inline void Resource::setAmount(uint8_t newAmount)
{
    if(getType() != Nothing)
        value = (value & 0xF0) | (newAmount & 0x0F);
}

#endif // Resource_h__
