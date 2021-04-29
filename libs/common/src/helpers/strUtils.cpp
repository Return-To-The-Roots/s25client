// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
