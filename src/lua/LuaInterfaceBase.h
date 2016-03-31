// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#include <string>

class LuaInterfaceBase{
public:

    LuaInterfaceBase();
    virtual ~LuaInterfaceBase();

    static void Register(kaguya::State& state);

    bool LoadScript(const std::string& scriptPath);
    bool LoadScriptString(const std::string& script);
    const std::string& GetScript() const { return script_; }

protected:
    kaguya::State lua;
    std::string script_;

    /// Write a string to log and stdout
    void Log(const std::string& msg);
    /// Return true, if local player is the host
    bool IsHost() const;
    /// Return the index of the local player
    unsigned GetLocalPlayerIdx() const;
    /// Show a message box with given title and text.
    /// If isError is true, a red exclamation mark is shown, otherwise a green one is shown
    void MsgBox(const std::string& title, const std::string& msg, bool isError);
    void MsgBox2(const std::string& title, const std::string& msg){ MsgBox(title, msg, false); }
    /// Shows a message with a custom icon. Image with iconIdx must exist in iconFile and iconFile must be loaded!
    void MsgBoxEx(const std::string& title, const std::string& msg, const std::string& iconFile, unsigned iconIdx);
    void MsgBoxEx2(const std::string& title, const std::string& msg, const std::string& iconFile, unsigned iconIdx, int iconX, int iconY);

    static void ErrorHandler(int status, const char* message);
    static void ErrorHandlerThrow(int status, const char* message);
};

#endif // LuaInterfaceBase_h__
