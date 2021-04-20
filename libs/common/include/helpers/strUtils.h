// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/lexical_cast.hpp>
#include <sstream>
#include <string>
#include <vector>

namespace helpers {

/// Convert to number and return true on success
template<typename T>
bool tryFromString(const std::string& value, T& outValue)
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
T fromString(const std::string& value, T defaultValue)
{
    T result;
    if(!tryFromString(value, result))
        result = defaultValue;
    return result;
}

/// Joins all values using delimiter. The last value is joined by using endDelimiter
std::string join(const std::vector<std::string>& values, const std::string& delimiter, const std::string& endDelimiter);
/// Joins all values using delimiter.
inline std::string join(const std::vector<std::string>& values, const std::string& delimiter)
{
    return join(values, delimiter, delimiter);
}

inline std::ostream& streamConcat(std::ostream& stream)
{
    return stream;
}

/// Concat arguments by passing them to a stream
template<typename Arg, typename... Args>
std::ostream& streamConcat(std::ostream& stream, Arg&& arg, Args&&... args)
{
    return streamConcat(stream << std::forward<Arg>(arg), std::forward<Args>(args)...);
}

template<class T_Stream = std::stringstream, typename... Args>
std::string concat(Args&&... args)
{
    T_Stream stream;
    streamConcat(stream, std::forward<Args>(args)...);
    return stream.str();
}

} // namespace helpers
