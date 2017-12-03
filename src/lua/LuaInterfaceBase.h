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
#include <map>
#include <string>

/// Base class for all lua script handlers
class LuaInterfaceBase
{
public:
    static void Register(kaguya::State& state);

    bool LoadScript(const std::string& scriptPath);
    bool LoadScriptString(const std::string& script);
    const std::string& GetScript() const { return script_; }

protected:
    LuaInterfaceBase();
    virtual ~LuaInterfaceBase();

    kaguya::State lua;
    std::string script_;

    bool ValidateUTF8(const std::string& scriptTxt);

    /// Write a string to log and stdout
    void Log(const std::string& msg);
    void RegisterTranslations(const kaguya::LuaRef& luaTranslations);

    std::string Translate(const std::string& key);

    static void ErrorHandler(int status, const char* message);
    static void ErrorHandlerThrow(int status, const char* message);

private:
    std::map<std::string, std::string> translations_;

    static std::map<std::string, std::string> GetTranslation(const kaguya::LuaRef& luaTranslations, const std::string& code);
};

#endif // LuaInterfaceBase_h__
