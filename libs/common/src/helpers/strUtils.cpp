// Copyright (c) 2018 - 2018 Settlers Freaks (sf-team at siedler25.org)
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

#include "helpers/strUtils.h"
#include <sstream>

namespace helpers {

std::string join(const std::vector<std::string>& values, const std::string& delimiter, const std::string& endDelimiter)
{
    if(values.empty())
        return "";
    if(values.size() == 1u)
        return values.front();
    std::ostringstream ss(values.front(), std::ostream::ate);
    for(auto it = values.begin() + 1; it != values.end() - 1; ++it)
        ss << delimiter << *it;
    ss << endDelimiter << values.back();
    return ss.str();
}

} // namespace helpers
