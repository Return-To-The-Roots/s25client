// Copyright (c) 2016 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef LuaBaseFixture_h__
#define LuaBaseFixture_h__

#include "GlobalVars.h"
#include "lua/LuaInterfaceGameBase.h"
#include <boost/format.hpp>
#include <boost/test/unit_test.hpp>

/// Provide lua execution methods and check if lua values equal given value
class LuaBaseFixture
{
private:
    LuaInterfaceGameBase* luaBase_;

protected:
    void setLua(LuaInterfaceGameBase* lua) { luaBase_ = lua; }

public:
    LuaBaseFixture() : luaBase_(nullptr) {}

    void executeLua(const std::string& luaCode) { luaBase_->LoadScriptString(luaCode, true); }
    void executeLua(const boost::format& luaCode) { executeLua(luaCode.str()); }
    kaguya::State& getLuaState() { return luaBase_->GetState(); }

    boost::test_tools::predicate_result isLuaEqual(const std::string& luaVal, const std::string& expectedValue)
    {
        try
        {
            executeLua(std::string("assert(") + luaVal + "==" + expectedValue + ", 'xxx=' .. tostring(" + luaVal + "))");
        } catch(std::runtime_error& e)
        {
            boost::test_tools::predicate_result result(false);
            std::string msg = e.what();
            size_t xPos = msg.rfind("xxx=");
            if(xPos != std::string::npos)
                result.message() << "Value = " << msg.substr(xPos + 4);
            else
                result.message() << e.what();
            return result;
        }
        return true;
    }
};

#endif // LuaBaseFixture_h__
