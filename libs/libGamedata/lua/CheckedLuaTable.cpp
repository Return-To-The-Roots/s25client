// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CheckedLuaTable.h"
#include "s25util/Log.h"
#include <boost/algorithm/string/join.hpp>
#include <algorithm>
#include <utility>

CheckedLuaTable::CheckedLuaTable(kaguya::LuaTable luaTable) : table(std::move(luaTable)), checkEnabled(false) {}

// NOLINTNEXTLINE(bugprone-exception-escape)
CheckedLuaTable::~CheckedLuaTable() noexcept(false)
{
    if(checkEnabled)
        checkUnused();
}

void CheckedLuaTable::checkUnused()
{
    checkEnabled = false;

    std::vector<std::string> tableKeys = table.keys<std::string>();
    std::sort(tableKeys.begin(), tableKeys.end());
    std::vector<std::string> unusedKeys;
    std::set_difference(tableKeys.begin(), tableKeys.end(), accessedKeys_.begin(), accessedKeys_.end(),
                        std::back_inserter(unusedKeys));
    for(const std::string& unusedKey : unusedKeys)
        LOG.write("\nERROR: Did not use key '%1%' in a lua table. This is most likely a bug!\n") % unusedKey;
    RTTR_Assert(unusedKeys.empty());
    if(!unusedKeys.empty())
        throw std::runtime_error("Did not use keys " + boost::algorithm::join(unusedKeys, ", ") + " in lua table!");
}
