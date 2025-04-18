// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "LuaInterfaceBase.h"
#include "helpers/strUtils.h"
#include "mygettext/LocaleInfo.h"
#include "mygettext/mygettext.h"
#include "mygettext/utils.h"
#include "s25util/Log.h"
#include "s25util/utf8.h"
#include <boost/nowide/convert.hpp>
#include <boost/nowide/detail/convert.hpp> // TODO: Remove when requiring Nowide 11 / Boost 1.74
#include <boost/nowide/fstream.hpp>
#include <algorithm>

LuaInterfaceBase::LuaInterfaceBase() : lua(kaguya::NoLoadLib()), logger_(LOG), errorOccured_(false)
{
    lua.openlib("base", luaopen_base);
    lua.openlib("package", luaopen_package);
    lua.openlib("string", luaopen_string);
    lua.openlib("table", luaopen_table);
    lua.openlib("math", luaopen_math);

    Register(lua);
    setThrowOnError(true);
    // Quasi-Standard translate function
    lua["_"] = kaguya::function([this](const std::string& s) { return translate(s); });
    // No-op translate (translated later)
    lua["__"] = gettext_noop;
}

LuaInterfaceBase::~LuaInterfaceBase() = default;

void LuaInterfaceBase::Register(kaguya::State& state)
{
    state["RTTRBase"].setClass(kaguya::UserdataMetatable<LuaInterfaceBase>()
                                 .addFunction("Log", &LuaInterfaceBase::log)
                                 .addFunction("RegisterTranslations", &LuaInterfaceBase::registerTranslations));
}

void LuaInterfaceBase::errorHandlerNoThrow(int /*status*/, const char* message)
{
    logger_.write(_("Lua error: %s\n")) % (message ? message : _("Unknown error"));
    errorOccured_ = true;
}

void LuaInterfaceBase::errorHandler(int status, const char* message)
{
    errorHandlerNoThrow(status, message);
    throw LuaExecutionError(message ? message : _("Unknown error"));
}

bool LuaInterfaceBase::loadScript(const boost::filesystem::path& scriptPath)
{
    boost::nowide::ifstream scriptFile(scriptPath);
    std::string tmpScript;
    tmpScript.assign(std::istreambuf_iterator<char>(scriptFile), std::istreambuf_iterator<char>());
    if(!scriptFile)
    {
        logger_.write(_("Failed to read lua file '%1%'\n")) % scriptPath;
        return false;
    }
    // Remove UTF-8 BOM if present
    if(tmpScript.substr(0, 3) == "\xef\xbb\xbf")
        tmpScript.erase(0, 3);
    if(!loadScriptString(tmpScript))
    {
        logger_.write(_("Failed to load lua file '%1%'\n")) % scriptPath;
        return false;
    }
    return true;
}

bool LuaInterfaceBase::loadScriptString(const std::string& script, bool rethrowError)
{
    script_.clear();
    if(!validateUTF8(script))
        return false;
    try
    {
        if(!lua.dostring(script))
            return false;
        else
            script_ = script;
    } catch(const LuaExecutionError&)
    {
        if(rethrowError)
            throw;
        return false;
    }
    return true;
}

void LuaInterfaceBase::setThrowOnError(bool doThrow)
{
    if(doThrow)
        lua.setErrorHandler([this](int status, const char* msg) { errorHandler(status, msg); });
    else
        lua.setErrorHandler([this](int status, const char* msg) { errorHandlerNoThrow(status, msg); });
}

std::map<std::string, std::string> LuaInterfaceBase::getTranslation(const kaguya::LuaRef& luaTranslations,
                                                                    const std::string& code)
{
    std::vector<std::string> folders = mygettext::getPossibleFoldersForLangCode(code);
    for(const std::string& folder : folders)
    {
        kaguya::LuaRef entry = luaTranslations[folder];
        if(entry.type() == LUA_TTABLE)
            return entry;
    }
    return std::map<std::string, std::string>();
}

void LuaInterfaceBase::registerTranslations(const kaguya::LuaRef& luaTranslations)
{
    // Init with default
    translations_ = getTranslation(luaTranslations, "en_GB");
    // Replace with entries of current locale
    const std::string locale = mygettext::setlocale(LC_ALL, nullptr);
    const std::map<std::string, std::string> translated = getTranslation(luaTranslations, locale);
    if(translated.empty())
    {
        boost::format fmt("Did not found translation for language '%1%' in LUA file. Available translations:  %2%\n");
        fmt % mygettext::LocaleInfo(locale).getName() % helpers::join(luaTranslations.keys<std::string>(), ", ");
        log(fmt.str());
    } else
    {
        for(const auto& it : translated)
            translations_[it.first] = it.second;
    }
}

std::string LuaInterfaceBase::translate(const std::string& key)
{
    const auto entry = translations_.find(key);
    if(entry == translations_.end())
        return key;
    else
        return entry->second;
}

bool LuaInterfaceBase::validateUTF8(const std::string& scriptTxt)
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
    const std::string faultyLine =
      boost::nowide::detail::convert_string<char>(&scriptTxt[lineBegin], &scriptTxt[lineEnd]);
    boost::format fmt("Found invalid UTF8 char at line %1%.\nContent: %2%\n");
    fmt % lineNum % faultyLine;
    log(fmt.str());
    return false;
}

void LuaInterfaceBase::log(const std::string& msg)
{
    logger_.write("%s\n") % msg;
}
