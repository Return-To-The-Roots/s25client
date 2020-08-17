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

#ifndef LuaInterfaceBase_h__
#define LuaInterfaceBase_h__

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

    static std::map<std::string, std::string> getTranslation(const kaguya::LuaRef& luaTranslations, const std::string& code);
};

#endif // LuaInterfaceBase_h__
