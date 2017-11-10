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

#ifndef converters_h__
#define converters_h__

#include <boost/lexical_cast.hpp>
#include <string>

namespace helpers {

/// Locale dependent string conversion
template<typename T>
inline std::string toString(const T& value)
{
    return boost::lexical_cast<std::string>(value);
}

/// Convert to number and return true on success
template<typename T>
inline bool tryFromString(const std::string& value, T& outValue)
{
    try
    {
        outValue = boost::lexical_cast<T>(value);
        return true;
    } catch(boost::bad_lexical_cast&)
    {
        return false;
    }
}

/// Convert to number or return defaultValue
template<typename T>
inline T fromString(const std::string& value, T defaultValue)
{
    T result;
    if(!tryFromString(value, result))
        result = defaultValue;
    return result;
}

} // namespace helpers

#endif // converters_h__
