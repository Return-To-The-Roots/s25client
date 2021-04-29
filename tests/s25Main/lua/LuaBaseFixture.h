// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "GlobalVars.h"
#include "lua/LuaInterfaceGameBase.h"
#include <boost/format.hpp>
#include <boost/test/unit_test.hpp>
#include <sstream>

/// Provide lua execution methods and check if lua values equal given value
class LuaBaseFixture
{
private:
    LuaInterfaceGameBase* luaBase_;

protected:
    void setLua(LuaInterfaceGameBase* lua) { luaBase_ = lua; }

public:
    LuaBaseFixture() : luaBase_(nullptr) {}

    void executeLua(const std::string& luaCode) { luaBase_->loadScriptString(luaCode, true); }
    void executeLua(const boost::format& luaCode) { executeLua(luaCode.str()); }
    kaguya::State& getLuaState() { return luaBase_->getState(); }

    boost::test_tools::predicate_result isLuaEqual(const std::string& luaVal, const std::string& expectedValue)
    {
        try
        {
            std::ostringstream ss;
            // Save to temporaries to run code only once
            ss << "_isVal = " << luaVal << "\n";
            ss << "_expectedVal = " << expectedValue << "\n";
            ss << "assert(_isVal == _expectedVal, 'xxx=' .. tostring(_isVal))";
            executeLua(ss.str());
        } catch(std::runtime_error& e)
        {
            boost::test_tools::predicate_result result(false);
            std::string msg = e.what();
            // Extract start of actual message to filter out the code that failed
            const auto msgPos = msg.rfind("\"]:1: ");
            if(msgPos != std::string::npos)
                msg = msg.substr(msgPos + 6);

            const auto xPos = msg.rfind("xxx=");
            if(xPos != std::string::npos)
                result.message() << "Value = " << msg.substr(xPos + 4);
            else
                result.message() << msg;
            return result;
        }
        return true;
    }
};
