// Copyright (c) 2005 - 2019 Settlers Freaks (sf-team at siedler25.org)
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
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "LuaInterfaceBase.h"

#include <commonDefines.h> // IWYU pragma: keep

#include <mygettext/mygettext.h>
#include <mygettext/utils.h>
#include <libutil/Log.h>

#include <utf8.h>
#include <boost/nowide/fstream.hpp>

#include <algorithm>

LuaInterfaceBase::LuaInterfaceBase()
    : lua_(kaguya::NoLoadLib())
    , errorOccured_(false)
{
    lua_.openlib("base", luaopen_base);
    lua_.openlib("package", luaopen_package);
    lua_.openlib("string", luaopen_string);
    lua_.openlib("table", luaopen_table);
    lua_.openlib("math", luaopen_math);

    Register(lua_);
    SetThrowOnError(true);
    lua_.setErrorHandler([this](int status, const char* msg) { ErrorHandler(status, msg); });

    // Quasi-Standard translate function
    lua_["_"] = kaguya::function([this](const std::string& s) { return Translate(s); });
    // No-op translate (translated later)
    lua_["__"] = gettext_noop;
}

LuaInterfaceBase::~LuaInterfaceBase() = default;

void LuaInterfaceBase::Register(kaguya::State& state)
{
    state["RTTRBase"].setClass(kaguya::UserdataMetatable<LuaInterfaceBase>()
                                 .addFunction("Log", &LuaInterfaceBase::Log)
                                 .addFunction("RegisterTranslations", &LuaInterfaceBase::RegisterTranslations));
}

void LuaInterfaceBase::ErrorHandlerNoThrow(int /*status*/, const char* message)
{
    LOG.write(_("Lua error: %s\n")) % (message ? message : "Unknown");
    errorOccured_ = true;
}

void LuaInterfaceBase::ErrorHandler(int status, const char* message)
{
    ErrorHandlerNoThrow(status, message);
    throw LuaExecutionError(message ? message : "Unknown error");
}

bool LuaInterfaceBase::LoadScript(const std::string& scriptPath)
{
    script_.clear();

    bnw::ifstream scriptFile(scriptPath.c_str());

    std::string tmpScript;
    tmpScript.assign(std::istreambuf_iterator<char>(scriptFile), std::istreambuf_iterator<char>());

    if(!scriptFile)
    {
        LOG.write("Failed to read lua file '%1%'\n") % scriptPath;
        return false;
    }

    if(!ValidateUTF8(tmpScript))
    {
        return false;
    }

    try
    {
        if(!lua_.dofile(scriptPath))
        {
            return false;
        }
        else
        {
            script_ = tmpScript;
        }
    }
    catch(const LuaExecutionError&)
    {
        return false;
    }
    return true;
}

bool LuaInterfaceBase::LoadScriptString(const std::string& script, bool rethrowError)
{
    script_.clear();

    if(!ValidateUTF8(script))
    {
        return false;
    }

    try
    {
        if(!lua_.dostring(script))
        {
            return false;
        }
        else
        {
            script_ = script;
        }
    }
    catch(const LuaExecutionError&)
    {
        if(rethrowError)
        {
            throw;
        }
        return false;
    }
    return true;
}

void LuaInterfaceBase::SetThrowOnError(bool doThrow)
{
    if(doThrow)
    {
        lua_.setErrorHandler([this](int status, const char* msg) { ErrorHandler(status, msg); });
    }
    else
    {
        lua_.setErrorHandler([this](int status, const char* msg) { ErrorHandlerNoThrow(status, msg); });
    }
}

std::map<std::string, std::string> LuaInterfaceBase::GetTranslation(const kaguya::LuaRef& luaTranslations, const std::string& code)
{
    const std::vector<std::string> folders = getPossibleFoldersForLangCode(code);
    
    for(const auto& folder : folders)
    {
        kaguya::LuaRef entry = luaTranslations[folder];
        if(entry.type() == LUA_TTABLE)
        {
            return entry;
        }
    }
    return std::map<std::string, std::string>();
}

void LuaInterfaceBase::RegisterTranslations(const kaguya::LuaRef& luaTranslations)
{
    // Init with default
    translations_ = GetTranslation(luaTranslations, "en_GB");

    // Replace with entries of current locale
    const std::string locale = mysetlocale(LC_ALL, nullptr);

    const std::map<std::string, std::string> translated = GetTranslation(luaTranslations, locale);

    for(const auto& it : translated)
    {
        translations_[it.first] = it.second;
    }
}

std::string LuaInterfaceBase::Translate(const std::string& key)
{
    const auto entry = translations_.find(key);
    if(entry == translations_.end())
    {
        return key;
    }
    else
    {
        return entry->second;
    }
}

bool LuaInterfaceBase::ValidateUTF8(const std::string& scriptTxt)
{
    const auto it = utf8::find_invalid(scriptTxt.begin(), scriptTxt.end());
    if(it == scriptTxt.end())
    {
        return true;
    }

    const size_t invPos = std::distance(scriptTxt.begin(), it);
    const size_t lineNum = std::count(scriptTxt.begin(), it, '\n') + 1;
    size_t lineBegin = scriptTxt.rfind('\n', invPos);
    size_t lineEnd = scriptTxt.find('\n', invPos);

    if(lineBegin == std::string::npos)
    {
        lineBegin = 0;
    }
    else
    {
        lineBegin++;
    }

    if(lineEnd == std::string::npos)
    {
        lineEnd = scriptTxt.size();
    }
    else
    {
        lineEnd--;
    }

    std::string faultyLine = scriptTxt.substr(lineBegin, lineEnd - lineBegin);
    std::string fixedLine;
    fixedLine.reserve(faultyLine.length());

    while(true) //-V776
    {
        try
        {
            utf8::replace_invalid(faultyLine.begin(), faultyLine.end(), std::back_inserter(fixedLine));
            break;
        }
        catch(utf8::not_enough_room&)
        {
            // Add zeroes until we have enough room (truncated UTF8) to detect the error
            faultyLine.push_back('\0');
            fixedLine.clear();
        }
    }

    boost::format fmt("Found invalid UTF8 char at line %1%.\nContent: %2%\n");
    fmt % lineNum % fixedLine;

    Log(fmt.str());

    return false;
}

void LuaInterfaceBase::Log(const std::string& msg)
{
    LOG.write("%s\n") % msg;
}
