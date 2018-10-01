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

#include "commonDefines.h" // IWYU pragma: keep
#include "CheckedLuaTable.h"
#include "boost/foreach.hpp"
#include "libutil/Log.h"
#include <boost/algorithm/string/join.hpp>
#include <algorithm>

CheckedLuaTable::CheckedLuaTable(const kaguya::LuaTable& luaTable) : table(luaTable), checkEnabled(false) {}

CheckedLuaTable::~CheckedLuaTable()
{
    if(checkEnabled)
        checkUnused(false);
}

bool CheckedLuaTable::checkUnused(bool throwError)
{
    checkEnabled = false;

    std::vector<std::string> tableKeys = table.keys<std::string>();
    std::sort(tableKeys.begin(), tableKeys.end());
    std::vector<std::string> unusedKeys;
    std::set_difference(tableKeys.begin(), tableKeys.end(), accessedKeys_.begin(), accessedKeys_.end(), std::back_inserter(unusedKeys));
    BOOST_FOREACH(const std::string& unusedKey, unusedKeys)
        LOG.write("\nERROR: Did not use key '%1%' in a lua table. This is most likely a bug!\n") % unusedKey;
    if(throwError)
    {
        RTTR_Assert(unusedKeys.empty());
        if(!unusedKeys.empty())
            throw std::runtime_error("Did not use keys " + boost::algorithm::join(unusedKeys, ", ") + " in lua table!");
    } else
    {
        // We should not throw errors in dtors
        RTTR_AssertNoThrow(unusedKeys.empty());
    }
    return unusedKeys.empty();
}
