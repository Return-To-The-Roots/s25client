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
#include "LuaInterfaceBase.h"
#include "helpers/strUtils.h"
#include "mygettext/LocaleInfo.h"
#include "mygettext/mygettext.h"
#include "mygettext/utils.h"
#include "s25util/Log.h"
#include "s25util/utf8.h"
#include <boost/nowide/convert.hpp>
#include <boost/nowide/fstream.hpp>
#include <algorithm>

LuaInterfaceBase::LuaInterfaceBase() : lua(kaguya::NoLoadLib()), errorOccured_(false)
{
    lua.openlib("base", luaopen_base);
    lua.openlib("package", luaopen_package);
    lua.openlib("string", luaopen_string);
    lua.openlib("table", luaopen_table);
    lua.openlib("math", luaopen_math);

    Register(lua);
    lua.setErrorHandler([this](int status, const char* msg) { ErrorHandler(status, msg); });
    // Quasi-Standard translate function
    lua["_"] = kaguya::function([this](const std::string& s) { return Translate(s); });
    // No-op translate (translated later)
    lua["__"] = gettext_noop;
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
    LOG.write(_("Lua error: %s\n")) % (message ? message : _("Unknown error"));
    errorOccured_ = true;
}

void LuaInterfaceBase::ErrorHandler(int status, const char* message)
{
    ErrorHandlerNoThrow(status, message);
    throw LuaExecutionError(message ? message : _("Unknown error"));
}

bool LuaInterfaceBase::LoadScript(const std::string& scriptPath)
{
    bnw::ifstream scriptFile(scriptPath.c_str());
    std::string tmpScript;
    tmpScript.assign(std::istreambuf_iterator<char>(scriptFile), std::istreambuf_iterator<char>());
    if(!scriptFile)
    {
        LOG.write(_("Failed to read lua file '%1%'\n")) % scriptPath;
        return false;
    }
    // Remove UTF-8 BOM if present
    if(tmpScript.substr(0, 3) == "\xef\xbb\xbf")
        tmpScript.erase(0, 3);
    if(!LoadScriptString(tmpScript))
    {
        LOG.write(_("Failed to load lua file '%1%'\n")) % scriptPath;
        return false;
    }
    return true;
}

bool LuaInterfaceBase::LoadScriptString(const std::string& script, bool rethrowError)
{
    script_.clear();
    if(!ValidateUTF8(script))
        return false;
    try
    {
        if(!lua.dostring(script))
            return false;
        else
            script_ = script;
    } catch(LuaExecutionError&)
    {
        if(rethrowError)
            throw;
        return false;
    }
    return true;
}

void LuaInterfaceBase::SetThrowOnError(bool doThrow)
{
    if(doThrow)
        lua.setErrorHandler([this](int status, const char* msg) { ErrorHandler(status, msg); });
    else
        lua.setErrorHandler([this](int status, const char* msg) { ErrorHandlerNoThrow(status, msg); });
}

std::map<std::string, std::string> LuaInterfaceBase::GetTranslation(const kaguya::LuaRef& luaTranslations, const std::string& code)
{
    std::vector<std::string> folders = getPossibleFoldersForLangCode(code);
    for(const std::string& folder : folders)
    {
        kaguya::LuaRef entry = luaTranslations[folder];
        if(entry.type() == LUA_TTABLE)
            return entry;
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
    if(translated.empty())
    {
        boost::format fmt("Did not found translation for language '%1%' in LUA file. Available translations:  %2%\n");
        fmt % LocaleInfo(locale).getName() % helpers::join(luaTranslations.keys<std::string>(), ", ");
        Log(fmt.str());
    } else
    {
        for(const auto& it : translated)
            translations_[it.first] = it.second;
    }
}

std::string LuaInterfaceBase::Translate(const std::string& key)
{
    const auto entry = translations_.find(key);
    if(entry == translations_.end())
        return key;
    else
        return entry->second;
}

bool LuaInterfaceBase::ValidateUTF8(const std::string& scriptTxt)
{
    const auto it = s25util::findInvalidUTF8(scriptTxt);
    if(it == scriptTxt.end())
        return true;
    size_t invPos = std::distance(scriptTxt.begin(), it);
    size_t lineNum = std::count(scriptTxt.begin(), it, '\n') + 1;
    size_t lineBegin = scriptTxt.rfind('\n', invPos);
    size_t lineEnd = scriptTxt.find('\n', invPos);
    if(lineBegin == std::string::npos)
        lineBegin = 0;
    else
        lineBegin++;
    if(lineEnd == std::string::npos)
        lineEnd = scriptTxt.size();
    else
        lineEnd--;
    const std::string faultyLine = boost::nowide::detail::convert_string<char>(&scriptTxt[lineBegin], &scriptTxt[lineEnd]);
    boost::format fmt("Found invalid UTF8 char at line %1%.\nContent: %2%\n");
    fmt % lineNum % faultyLine;
    Log(fmt.str());
    return false;
}

void LuaInterfaceBase::Log(const std::string& msg)
{
    LOG.write("%s\n") % msg;
}
