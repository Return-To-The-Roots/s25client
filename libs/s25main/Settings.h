// Copyright (c) 2005 - 2018 Settlers Freaks (sf-team at siedler25.org)
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

#pragma once

#include "driver/VideoMode.h"
#include "s25util/ProxySettings.h"
#include "s25util/Singleton.h"
#include <boost/optional.hpp>
#include <array>
#include <map>
#include <string>

#undef interface

namespace validate {
boost::optional<uint16_t> checkPort(const std::string& port);
bool checkPort(int port);
} // namespace validate

/// Configuration class
class Settings : public Singleton<Settings, SingletonPolicies::WithLongevity>
{
public:
    static constexpr unsigned Longevity = 18;

    Settings();

    void Load();
    void Save();

protected:
    void LoadDefaults();

public:
    struct
    {
        unsigned submit_debug_data;
        unsigned use_upnp;
        bool smartCursor;
        bool debugMode;
    } global;

    struct
    {
        VideoMode fullscreenSize, windowedSize;
        signed short vsync; // <0 for unlimited, 0 for HW Vsync
        bool fullscreen;
        bool vbo;
        bool shared_textures;
    } video;

    struct
    {
        std::string language;
    } language;

    struct
    {
        std::string audio;
        std::string video;
    } driver;

    struct
    {
        bool musik;
        unsigned char musik_volume;
        bool effekte;
        unsigned char effekte_volume;
        std::string playlist; /// musicplayer playlist name
    } sound;

    struct
    {
        std::string name;
        std::string password;
        bool save_password;
    } lobby;

    struct
    {
        std::string last_ip; /// last entered ip or hostname
        uint16_t localPort;
        bool ipv6; /// listen/connect on ipv6 as default or not
    } server;

    ProxySettings proxy;

    struct
    {
        unsigned autosave_interval;
        bool revert_mouse;
    } interface;

    struct
    {
        bool scale_statistics;
    } ingame;

    struct
    {
        std::map<unsigned, unsigned> configuration;
    } addons;

    static const std::array<short, 13> SCREEN_REFRESH_RATES;

private:
    static const int VERSION;
    static const std::array<std::string, 11> SECTION_NAMES;
};

#define SETTINGS Settings::inst()
