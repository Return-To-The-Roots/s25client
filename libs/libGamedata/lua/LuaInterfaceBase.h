// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <kaguya/kaguya.hpp>
#include <boost/filesystem/path.hpp>
#include <map>
#include <stdexcept>
#include <string>

namespace kaguya {
class State;
}
class Log;

class LuaExecutionError : public std::runtime_error
{
public:
    LuaExecutionError(const std::string& msg) : std::runtime_error(msg) {}
};

/// Base class for all lua script handlers
class LuaInterfaceBase
{
public:
    static void Register(kaguya::State& state);

    bool loadScript(const boost::filesystem::path& scriptPath);
    bool loadScriptString(const std::string& script, bool rethrowError = false);
    const std::string& getScript() const { return script_; }
    /// Disable or re-enable throwing an exception on error.
    /// Note: If error throwing is disabled you have to use HasErrorOccurred to detect an error situation
    void setThrowOnError(bool doThrow);
    bool hasErrorOccurred() const { return errorOccured_; }
    void clearErrorOccured() { errorOccured_ = false; }

    kaguya::State& getState() { return lua; }

protected:
    LuaInterfaceBase();
    virtual ~LuaInterfaceBase();

    kaguya::State lua;
    std::string script_;

    bool validateUTF8(const std::string& scriptTxt);

    /// Write a string to log and stdout
    void log(const std::string& msg);
    void registerTranslations(const kaguya::LuaRef& luaTranslations);

    std::string translate(const std::string& key);

    void errorHandlerNoThrow(int status, const char* message);
    [[noreturn]] void errorHandler(int status, const char* message);

private:
    Log& logger_;
    /// Sticky flag to signal an occurred error during execution of lua code
    bool errorOccured_;
    std::map<std::string, std::string> translations_;

    static std::map<std::string, std::string> getTranslation(const kaguya::LuaRef& luaTranslations,
                                                             const std::string& code);
};
